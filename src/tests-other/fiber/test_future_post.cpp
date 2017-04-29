//  (C) Copyright 2008-10 Anthony Williams 
//                2015    Oliver Kowalke
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/test/included/unit_test.hpp>

#include <memoria/v1/fiber/all.hpp>

typedef std::chrono::milliseconds ms;
typedef std::chrono::high_resolution_clock Clock;

int gi = 7;

struct my_exception : public std::runtime_error {
    my_exception() :
        std::runtime_error("my_exception") {
    }
};

struct A {
    A() = default;

    A( A const&) = delete;
    A( A &&) = default;

    A & operator=( A const&) = delete;
    A & operator=( A &&) = default;

    int value;
};

void fn1( memoria::v1::fibers::promise< int > * p, int i) {
    memoria::v1::this_fiber::yield();
    p->set_value( i);
}

void fn2() {
    memoria::v1::fibers::promise< int > p;
    memoria::v1::fibers::future< int > f( p.get_future() );
    memoria::v1::this_fiber::yield();
    memoria::v1::fibers::fiber( memoria::v1::fibers::launch::post, fn1, & p, 7).detach();
    memoria::v1::this_fiber::yield();
    BOOST_CHECK( 7 == f.get() );
}

int fn3() {
    return 3;
}

void fn4() {
}

int fn5() {
    boost::throw_exception( my_exception() );
    return 3;
}

void fn6() {
    boost::throw_exception( my_exception() );
}

int & fn7() {
    return gi;
}

int fn8( int i) {
    return i;
}

A fn9() {
     A a;
     a.value = 3;
     return std::move( a);
}

A fn10() {
    boost::throw_exception( my_exception() );
    return A();
}

void fn11( memoria::v1::fibers::promise< int > p) {
  memoria::v1::this_fiber::sleep_for( ms(500) );
  p.set_value(3);
}

void fn12( memoria::v1::fibers::promise< int& > p) {
  memoria::v1::this_fiber::sleep_for( ms(500) );
  gi = 5;
  p.set_value( gi);
}

void fn13( memoria::v1::fibers::promise< void > p) {
  memoria::v1::this_fiber::sleep_for( ms(500) );
  p.set_value();
}

// future
void test_future_create() {
    // default constructed future is not valid
    memoria::v1::fibers::future< int > f1;
    BOOST_CHECK( ! f1.valid() );

    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int > p2;
    memoria::v1::fibers::future< int > f2 = p2.get_future();
    BOOST_CHECK( f2.valid() );
}

void test_future_create_ref() {
    // default constructed future is not valid
    memoria::v1::fibers::future< int& > f1;
    BOOST_CHECK( ! f1.valid() );

    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int& > p2;
    memoria::v1::fibers::future< int& > f2 = p2.get_future();
    BOOST_CHECK( f2.valid() );
}

void test_future_create_void() {
    // default constructed future is not valid
    memoria::v1::fibers::future< void > f1;
    BOOST_CHECK( ! f1.valid() );

    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< void > p2;
    memoria::v1::fibers::future< void > f2 = p2.get_future();
    BOOST_CHECK( f2.valid() );
}

void test_future_move() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int > p1;
    memoria::v1::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // move construction
    memoria::v1::fibers::future< int > f2( std::move( f1) );
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = std::move( f2);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2.valid() );
}

void test_future_move_ref() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int& > p1;
    memoria::v1::fibers::future< int& > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // move construction
    memoria::v1::fibers::future< int& > f2( std::move( f1) );
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = std::move( f2);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2.valid() );
}

void test_future_move_void() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< void > p1;
    memoria::v1::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // move construction
    memoria::v1::fibers::future< void > f2( std::move( f1) );
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = std::move( f2);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2.valid() );
}

void test_future_get() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int > p1;
    p1.set_value( 7);

    memoria::v1::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // get
    BOOST_CHECK( ! f1.get_exception_ptr() );
    BOOST_CHECK( 7 == f1.get() );
    BOOST_CHECK( ! f1.valid() );

    // throw broken_promise if promise is destroyed without set
    {
        memoria::v1::fibers::promise< int > p2;
        f1 = p2.get_future();
    }
    bool thrown = false;
    try {
        f1.get();
    } catch ( memoria::v1::fibers::broken_promise const&) {
        thrown = true;
    }
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( thrown);
}

void test_future_get_move() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< A > p1;
    A a; a.value = 7;
    p1.set_value( std::move( a) );

    memoria::v1::fibers::future< A > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // get
    BOOST_CHECK( ! f1.get_exception_ptr() );
    BOOST_CHECK( 7 == f1.get().value);
    BOOST_CHECK( ! f1.valid() );

    // throw broken_promise if promise is destroyed without set
    {
        memoria::v1::fibers::promise< A > p2;
        f1 = p2.get_future();
    }
    bool thrown = false;
    try {
        f1.get();
    } catch ( memoria::v1::fibers::broken_promise const&) {
        thrown = true;
    }
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( thrown);
}

void test_future_get_ref() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int& > p1;
    int i = 7;
    p1.set_value( i);

    memoria::v1::fibers::future< int& > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // get
    BOOST_CHECK( ! f1.get_exception_ptr() );
    int & j = f1.get();
    BOOST_CHECK( &i == &j);
    BOOST_CHECK( ! f1.valid() );

    // throw broken_promise if promise is destroyed without set
    {
        memoria::v1::fibers::promise< int& > p2;
        f1 = p2.get_future();
    }
    bool thrown = false;
    try {
        f1.get();
    } catch ( memoria::v1::fibers::broken_promise const&) {
        thrown = true;
    }
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( thrown);
}


void test_future_get_void() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< void > p1;
    p1.set_value();

    memoria::v1::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // get
    BOOST_CHECK( ! f1.get_exception_ptr() );
    f1.get();
    BOOST_CHECK( ! f1.valid() );

    // throw broken_promise if promise is destroyed without set
    {
        memoria::v1::fibers::promise< void > p2;
        f1 = p2.get_future();
    }
    bool thrown = false;
    try {
        f1.get();
    } catch ( memoria::v1::fibers::broken_promise const&) {
        thrown = true;
    }
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( thrown);
}

void test_future_share() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int > p1;
    int i = 7;
    p1.set_value( i);

    memoria::v1::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // share
    memoria::v1::fibers::shared_future< int > sf1 = f1.share();
    BOOST_CHECK( sf1.valid() );
    BOOST_CHECK( ! f1.valid() );

    // get
    BOOST_CHECK( ! sf1.get_exception_ptr() );
    int j = sf1.get();
    BOOST_CHECK_EQUAL( i, j);
    BOOST_CHECK( sf1.valid() );
}

void test_future_share_ref() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int& > p1;
    int i = 7;
    p1.set_value( i);

    memoria::v1::fibers::future< int& > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // share
    memoria::v1::fibers::shared_future< int& > sf1 = f1.share();
    BOOST_CHECK( sf1.valid() );
    BOOST_CHECK( ! f1.valid() );

    // get
    BOOST_CHECK( ! sf1.get_exception_ptr() );
    int & j = sf1.get();
    BOOST_CHECK( &i == &j);
    BOOST_CHECK( sf1.valid() );
}

void test_future_share_void() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< void > p1;
    p1.set_value();

    memoria::v1::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // share
    memoria::v1::fibers::shared_future< void > sf1 = f1.share();
    BOOST_CHECK( sf1.valid() );
    BOOST_CHECK( ! f1.valid() );

    // get
    BOOST_CHECK( ! sf1.get_exception_ptr() );
    sf1.get();
    BOOST_CHECK( sf1.valid() );
}

void test_future_wait() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int > p1;
    memoria::v1::fibers::future< int > f1 = p1.get_future();

    // wait on future
    p1.set_value( 7);
    f1.wait();
    BOOST_CHECK( 7 == f1.get() );
}

void test_future_wait_ref() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int& > p1;
    memoria::v1::fibers::future< int& > f1 = p1.get_future();

    // wait on future
    int i = 7;
    p1.set_value( i);
    f1.wait();
    int & j = f1.get();
    BOOST_CHECK( &i == &j);
}

void test_future_wait_void() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< void > p1;
    memoria::v1::fibers::future< void > f1 = p1.get_future();

    // wait on future
    p1.set_value();
    f1.wait();
    f1.get();
    BOOST_CHECK( ! f1.valid() );
}

void test_future_wait_for() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int > p1;
    memoria::v1::fibers::future< int > f1 = p1.get_future();

    memoria::v1::fibers::fiber( memoria::v1::fibers::launch::post, fn11, std::move( p1) ).detach();

    // wait on future
    BOOST_CHECK( f1.valid() );
    memoria::v1::fibers::future_status status = f1.wait_for( ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::timeout == status);

    BOOST_CHECK( f1.valid() );
    status = f1.wait_for( ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::ready == status);

    BOOST_CHECK( f1.valid() );
    f1.wait();
}

void test_future_wait_for_ref() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int& > p1;
    memoria::v1::fibers::future< int& > f1 = p1.get_future();

    memoria::v1::fibers::fiber( memoria::v1::fibers::launch::post, fn12, std::move( p1) ).detach();

    // wait on future
    BOOST_CHECK( f1.valid() );
    memoria::v1::fibers::future_status status = f1.wait_for( ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::timeout == status);

    BOOST_CHECK( f1.valid() );
    status = f1.wait_for( ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::ready == status);

    BOOST_CHECK( f1.valid() );
    f1.wait();
}

void test_future_wait_for_void() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< void > p1;
    memoria::v1::fibers::future< void > f1 = p1.get_future();

    memoria::v1::fibers::fiber( memoria::v1::fibers::launch::post, fn13, std::move( p1) ).detach();

    // wait on future
    BOOST_CHECK( f1.valid() );
    memoria::v1::fibers::future_status status = f1.wait_for( ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::timeout == status);

    BOOST_CHECK( f1.valid() );
    status = f1.wait_for( ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::ready == status);

    BOOST_CHECK( f1.valid() );
    f1.wait();
}

void test_future_wait_until() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int > p1;
    memoria::v1::fibers::future< int > f1 = p1.get_future();

    memoria::v1::fibers::fiber( memoria::v1::fibers::launch::post, fn11, std::move( p1) ).detach();

    // wait on future
    BOOST_CHECK( f1.valid() );
    memoria::v1::fibers::future_status status = f1.wait_until( Clock::now() + ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::timeout == status);

    BOOST_CHECK( f1.valid() );
    status = f1.wait_until( Clock::now() + ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::ready == status);

    BOOST_CHECK( f1.valid() );
    f1.wait();
}

void test_future_wait_until_ref() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< int& > p1;
    memoria::v1::fibers::future< int& > f1 = p1.get_future();

    memoria::v1::fibers::fiber( memoria::v1::fibers::launch::post, fn12, std::move( p1) ).detach();

    // wait on future
    BOOST_CHECK( f1.valid() );
    memoria::v1::fibers::future_status status = f1.wait_until( Clock::now() + ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::timeout == status);

    BOOST_CHECK( f1.valid() );
    status = f1.wait_until( Clock::now() + ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::ready == status);

    BOOST_CHECK( f1.valid() );
    f1.wait();
}

void test_future_wait_until_void() {
    // future retrieved from promise is valid (if it is the first)
    memoria::v1::fibers::promise< void > p1;
    memoria::v1::fibers::future< void > f1 = p1.get_future();

    memoria::v1::fibers::fiber( memoria::v1::fibers::launch::post, fn13, std::move( p1) ).detach();

    // wait on future
    BOOST_CHECK( f1.valid() );
    memoria::v1::fibers::future_status status = f1.wait_until( Clock::now() + ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::timeout == status);

    BOOST_CHECK( f1.valid() );
    status = f1.wait_until( Clock::now() + ms(300) );
    BOOST_CHECK( memoria::v1::fibers::future_status::ready == status);

    BOOST_CHECK( f1.valid() );
    f1.wait();
}

void test_future_wait_with_fiber_1() {
    memoria::v1::fibers::promise< int > p1;
    memoria::v1::fibers::fiber( memoria::v1::fibers::launch::post, fn1, & p1, 7).detach();

    memoria::v1::fibers::future< int > f1 = p1.get_future();

    // wait on future
    BOOST_CHECK( 7 == f1.get() );
}

void test_future_wait_with_fiber_2() {
    memoria::v1::fibers::fiber( memoria::v1::fibers::launch::post, fn2).join();
}


boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[]) {
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Fiber: future test suite");

    test->add(BOOST_TEST_CASE(test_future_create));
    test->add(BOOST_TEST_CASE(test_future_create_ref));
    test->add(BOOST_TEST_CASE(test_future_create_void));
    test->add(BOOST_TEST_CASE(test_future_move));
    test->add(BOOST_TEST_CASE(test_future_move_ref));
    test->add(BOOST_TEST_CASE(test_future_move_void));
    test->add(BOOST_TEST_CASE(test_future_get));
    test->add(BOOST_TEST_CASE(test_future_get_move));
    test->add(BOOST_TEST_CASE(test_future_get_ref));
    test->add(BOOST_TEST_CASE(test_future_get_void));
    test->add(BOOST_TEST_CASE(test_future_share));
    test->add(BOOST_TEST_CASE(test_future_share_ref));
    test->add(BOOST_TEST_CASE(test_future_share_void));
    test->add(BOOST_TEST_CASE(test_future_wait));
    test->add(BOOST_TEST_CASE(test_future_wait_ref));
    test->add(BOOST_TEST_CASE(test_future_wait_void));
    test->add(BOOST_TEST_CASE(test_future_wait_for));
    test->add(BOOST_TEST_CASE(test_future_wait_for_ref));
    test->add(BOOST_TEST_CASE(test_future_wait_for_void));
    test->add(BOOST_TEST_CASE(test_future_wait_until));
    test->add(BOOST_TEST_CASE(test_future_wait_until_ref));
    test->add(BOOST_TEST_CASE(test_future_wait_until_void));
    test->add(BOOST_TEST_CASE(test_future_wait_with_fiber_1));
    test->add(BOOST_TEST_CASE(test_future_wait_with_fiber_2));

    return test;
}