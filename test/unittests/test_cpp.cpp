// EVMC: Ethereum Client-VM Connector API.
// Copyright 2018-2019 The EVMC Authors.
// Licensed under the Apache License, Version 2.0.

// The vector is not used here, but including it was causing compilation issues
// previously related to using explicit template argument (SFINAE disabled).
#include <vector>

#include "../../examples/example_host.h"
#include "../../examples/example_vm/example_vm.h"

#include <evmc/evmc.hpp>
#include <evmc/helpers.hpp>

#include <gtest/gtest.h>

#include <cstring>


TEST(cpp, address)
{
    evmc::address a;
    EXPECT_EQ(std::count(std::begin(a.bytes), std::end(a.bytes), 0), sizeof(a));

    auto other = evmc_address{};
    other.bytes[19] = 0xfe;
    a = other;
    EXPECT_TRUE(std::equal(std::begin(a.bytes), std::end(a.bytes), std::begin(other.bytes)));

    a.bytes[0] = 1;
    other = a;
    EXPECT_TRUE(std::equal(std::begin(a.bytes), std::end(a.bytes), std::begin(other.bytes)));
}

TEST(cpp, bytes32)
{
    evmc::bytes32 b;
    EXPECT_EQ(std::count(std::begin(b.bytes), std::end(b.bytes), 0), sizeof(b));

    auto other = evmc_bytes32{};
    other.bytes[31] = 0xfe;
    b = other;
    EXPECT_TRUE(std::equal(std::begin(b.bytes), std::end(b.bytes), std::begin(other.bytes)));

    b.bytes[0] = 1;
    other = b;
    EXPECT_TRUE(std::equal(std::begin(b.bytes), std::end(b.bytes), std::begin(other.bytes)));
}

TEST(cpp, std_hash)
{
#pragma warning(push)
#pragma warning(disable : 4307 /* integral constant overflow */)
#pragma warning(disable : 4309 /* 'static_cast': truncation of constant value */)

#if !defined(_MSC_VER) || (_MSC_VER >= 1910 /* Only for Visual Studio 2017+ */)
    static_assert(std::hash<evmc::address>{}({}) == static_cast<size_t>(0xd94d12186c0f2fb7), "");
    static_assert(std::hash<evmc::bytes32>{}({}) == static_cast<size_t>(0x4d25767f9dce13f5), "");
#endif

    EXPECT_EQ(std::hash<evmc::address>{}({}), static_cast<size_t>(0xd94d12186c0f2fb7));
    EXPECT_EQ(std::hash<evmc::bytes32>{}({}), static_cast<size_t>(0x4d25767f9dce13f5));

    auto ea = evmc::address{};
    std::fill_n(ea.bytes, sizeof(ea), uint8_t{0xee});
    EXPECT_EQ(std::hash<evmc::address>{}(ea), static_cast<size_t>(0x41dc0178e01b7cd9));

    auto eb = evmc::bytes32{};
    std::fill_n(eb.bytes, sizeof(eb), uint8_t{0xee});
    EXPECT_EQ(std::hash<evmc::bytes32>{}(eb), static_cast<size_t>(0xbb14e5c56b477375));

#pragma warning(pop)
}

TEST(cpp, address_comparison)
{
    const auto zero = evmc::address{};
    for (size_t i = 0; i < sizeof(evmc::address); ++i)
    {
        auto t = evmc::address{};
        t.bytes[i] = 1;
        auto u = evmc::address{};
        u.bytes[i] = 2;
        auto f = evmc::address{};
        f.bytes[i] = 0xff;

        EXPECT_TRUE(zero < t);
        EXPECT_TRUE(zero < u);
        EXPECT_TRUE(zero < f);
        EXPECT_TRUE(zero != t);
        EXPECT_TRUE(zero != u);
        EXPECT_TRUE(zero != f);

        EXPECT_TRUE(t < u);
        EXPECT_TRUE(t < f);
        EXPECT_TRUE(u < f);

        EXPECT_FALSE(u < t);
        EXPECT_FALSE(f < t);
        EXPECT_FALSE(f < u);

        EXPECT_TRUE(t != u);
        EXPECT_TRUE(t != f);
        EXPECT_TRUE(u != t);
        EXPECT_TRUE(u != f);
        EXPECT_TRUE(f != t);
        EXPECT_TRUE(f != u);

        EXPECT_TRUE(t == t);
        EXPECT_TRUE(u == u);
        EXPECT_TRUE(f == f);
    }
}

TEST(cpp, bytes32_comparison)
{
    const auto zero = evmc::bytes32{};
    for (size_t i = 0; i < sizeof(evmc::bytes32); ++i)
    {
        auto t = evmc::bytes32{};
        t.bytes[i] = 1;
        auto u = evmc::bytes32{};
        u.bytes[i] = 2;
        auto f = evmc::bytes32{};
        f.bytes[i] = 0xff;

        EXPECT_TRUE(zero < t);
        EXPECT_TRUE(zero < u);
        EXPECT_TRUE(zero < f);
        EXPECT_TRUE(zero != t);
        EXPECT_TRUE(zero != u);
        EXPECT_TRUE(zero != f);

        EXPECT_TRUE(t < u);
        EXPECT_TRUE(t < f);
        EXPECT_TRUE(u < f);

        EXPECT_FALSE(u < t);
        EXPECT_FALSE(f < t);
        EXPECT_FALSE(f < u);

        EXPECT_TRUE(t != u);
        EXPECT_TRUE(t != f);
        EXPECT_TRUE(u != t);
        EXPECT_TRUE(u != f);
        EXPECT_TRUE(f != t);
        EXPECT_TRUE(f != u);

        EXPECT_TRUE(t == t);
        EXPECT_TRUE(u == u);
        EXPECT_TRUE(f == f);
    }
}

TEST(cpp, result)
{
    static int release_called = 0;
    release_called = 0;

    {
        EXPECT_EQ(release_called, 0);
        auto raw_result = evmc_result{};
        raw_result.output_data = static_cast<uint8_t*>(std::malloc(13));
        raw_result.release = [](const evmc_result* r) {
            std::free(const_cast<uint8_t*>(r->output_data));
            ++release_called;
        };

        auto res1 = evmc::result{raw_result};
        auto res2 = std::move(res1);

        EXPECT_EQ(release_called, 0);

        [](evmc::result) {}(std::move(res2));

        EXPECT_EQ(release_called, 1);
    }
    EXPECT_EQ(release_called, 1);
}

TEST(cpp, vm)
{
    auto vm = evmc::vm{evmc_create_example_vm()};
    EXPECT_TRUE(vm.is_abi_compatible());

    auto r = vm.set_option("verbose", "3");
    EXPECT_EQ(r, EVMC_SET_OPTION_SUCCESS);

    EXPECT_EQ(vm.name(), std::string{"example_vm"});
    EXPECT_NE(vm.version()[0], 0);

    auto ctx = evmc_context{};
    auto res = vm.execute(ctx, EVMC_MAX_REVISION, {}, nullptr, 0);
    EXPECT_EQ(res.status_code, EVMC_FAILURE);

    EXPECT_TRUE(vm.get_capabilities() & EVMC_CAPABILITY_EVM1);
}

TEST(cpp, vm_set_option)
{
    evmc_instance raw_instance = {EVMC_ABI_VERSION, "",      "",      nullptr,
                                  nullptr,          nullptr, nullptr, nullptr};
    raw_instance.destroy = [](evmc_instance*) {};

    auto vm = evmc::vm{&raw_instance};
    EXPECT_EQ(vm.set_option("1", "2"), EVMC_SET_OPTION_INVALID_NAME);
}

TEST(cpp, vm_null)
{
    evmc::vm vm;
    EXPECT_FALSE(vm);
    EXPECT_TRUE(!vm);
}

TEST(cpp, vm_move)
{
    static int destroy_counter = 0;
    const auto template_instance =
        evmc_instance{EVMC_ABI_VERSION, "",      "",      [](evmc_instance*) { ++destroy_counter; },
                      nullptr,          nullptr, nullptr, nullptr};

    EXPECT_EQ(destroy_counter, 0);
    {
        auto v1 = template_instance;
        auto v2 = template_instance;

        auto vm1 = evmc::vm{&v1};
        EXPECT_TRUE(vm1);
        vm1 = evmc::vm{&v2};
        EXPECT_TRUE(vm1);
    }
    EXPECT_EQ(destroy_counter, 2);
    {
        auto v1 = template_instance;

        auto vm1 = evmc::vm{&v1};
        EXPECT_TRUE(vm1);
        vm1 = evmc::vm{};
        EXPECT_FALSE(vm1);
    }
    EXPECT_EQ(destroy_counter, 3);
    {
        auto v1 = template_instance;

        auto vm1 = evmc::vm{&v1};
        EXPECT_TRUE(vm1);
        auto vm2 = std::move(vm1);
        EXPECT_TRUE(vm2);
        EXPECT_FALSE(vm1);  // NOLINT
        auto vm3 = std::move(vm2);
        EXPECT_TRUE(vm3);
        EXPECT_FALSE(vm2);  // NOLINT
        EXPECT_FALSE(vm1);
    }
    EXPECT_EQ(destroy_counter, 4);
    {
        // Moving to itself will destroy the VM and reset the evmc::vm.
        auto v1 = template_instance;

        auto vm1 = evmc::vm{&v1};
        auto& vm1_ref = vm1;
        vm1 = std::move(vm1_ref);
        EXPECT_EQ(destroy_counter, 5);  // Already destroyed.
        EXPECT_FALSE(vm1);              // Null.
    }
    EXPECT_EQ(destroy_counter, 5);
}

TEST(cpp, host)
{
    // Use example host to execute all methods from the C++ host wrapper.

    auto* host_context = example_host_create_context();
    auto host = evmc::HostContext{host_context};

    auto a = evmc_address{{1}};
    auto v = evmc_bytes32{{7, 7, 7}};

    EXPECT_FALSE(host.account_exists(a));

    EXPECT_EQ(host.set_storage(a, {}, v), EVMC_STORAGE_MODIFIED);
    EXPECT_EQ(host.set_storage(a, {}, v), EVMC_STORAGE_UNCHANGED);
    EXPECT_EQ(host.get_storage(a, {}), v);

    EXPECT_TRUE(is_zero(host.get_balance(a)));

    EXPECT_EQ(host.get_code_size(a), 0);
    EXPECT_EQ(host.get_code_hash(a), evmc_bytes32{});
    EXPECT_EQ(host.copy_code(a, 0, nullptr, 0), 0);

    host.selfdestruct(a, a);

    auto tx = host.get_tx_context();
    EXPECT_EQ(host.get_tx_context().block_number, tx.block_number);

    EXPECT_EQ(host.get_block_hash(0), evmc_bytes32{});

    host.emit_log(a, nullptr, 0, nullptr, 0);

    example_host_destroy_context(host_context);
}

TEST(cpp, host_call)
{
    // Use example host to test Host::call() method.

    auto* host_context = example_host_create_context();
    auto host = evmc::HostContext{host_context};

    EXPECT_EQ(host.call({}).gas_left, 0);

    auto msg = evmc_message{};
    msg.gas = 1;
    uint8_t input[] = {0xa, 0xb, 0xc};
    msg.input_data = input;
    msg.input_size = sizeof(input);

    auto res = host.call(msg);
    EXPECT_EQ(res.status_code, EVMC_REVERT);
    EXPECT_EQ(res.output_size, msg.input_size);
    EXPECT_TRUE(std::equal(&res.output_data[0], &res.output_data[res.output_size], msg.input_data));

    example_host_destroy_context(host_context);
}

TEST(cpp, result_raii)
{
    static auto release_called = 0;
    release_called = 0;
    auto release_fn = [](const evmc_result*) noexcept { ++release_called; };

    {
        auto raw_result = evmc_result{};
        raw_result.status_code = EVMC_INTERNAL_ERROR;
        raw_result.release = release_fn;

        auto raii_result = evmc::result{raw_result};
        EXPECT_EQ(raii_result.status_code, EVMC_INTERNAL_ERROR);
        EXPECT_EQ(raii_result.gas_left, 0);
        raii_result.gas_left = -1;

        auto raw_result2 = raii_result.release_raw();
        EXPECT_EQ(raw_result2.status_code, EVMC_INTERNAL_ERROR);
        EXPECT_EQ(raw_result.status_code, EVMC_INTERNAL_ERROR);
        EXPECT_EQ(raw_result2.gas_left, -1);
        EXPECT_EQ(raw_result.gas_left, 0);
        EXPECT_EQ(raw_result2.release, release_fn);
        EXPECT_EQ(raw_result.release, release_fn);
    }
    EXPECT_EQ(release_called, 0);

    {
        auto raw_result = evmc_result{};
        raw_result.status_code = EVMC_INTERNAL_ERROR;
        raw_result.release = release_fn;

        auto raii_result = evmc::result{raw_result};
        EXPECT_EQ(raii_result.status_code, EVMC_INTERNAL_ERROR);
    }
    EXPECT_EQ(release_called, 1);
}

TEST(cpp, result_move)
{
    static auto release_called = 0;
    auto release_fn = [](const evmc_result*) noexcept { ++release_called; };

    release_called = 0;
    {
        auto raw = evmc_result{};
        raw.gas_left = -1;
        raw.release = release_fn;

        auto r0 = evmc::result{raw};
        EXPECT_EQ(r0.gas_left, raw.gas_left);

        auto r1 = std::move(r0);
        EXPECT_EQ(r1.gas_left, raw.gas_left);
    }
    EXPECT_EQ(release_called, 1);

    release_called = 0;
    {
        auto raw1 = evmc_result{};
        raw1.gas_left = 1;
        raw1.release = release_fn;

        auto raw2 = evmc_result{};
        raw2.gas_left = 1;
        raw2.release = release_fn;

        auto r1 = evmc::result{raw1};
        auto r2 = evmc::result{raw2};

        r2 = std::move(r1);
    }
    EXPECT_EQ(release_called, 2);
}
