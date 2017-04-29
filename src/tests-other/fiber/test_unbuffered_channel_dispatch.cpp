
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <chrono>
#include <sstream>
#include <string>
#include <vector>

#include <boost/assert.hpp>
#include <boost/test/included/unit_test.hpp>

#include <memoria/v1/fiber/all.hpp>

struct moveable {
    bool    state;
    int     value;

    moveable() :
        state( false),
        value( -1) {
    }

    moveable( int v) :
        state( true),
        value( v) {
    }

    moveable( moveable && other) :
        state( other.state),
        value( other.value) {
        other.state = false;
        other.value = -1;
    }

    moveable & operator=( moveable && other) {
        if ( this == & other) return * this;
        state = other.state;
        other.state = false;
        value = other.value;
        other.value = -1;
        return * this;
    }
};

void test_push() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( 1) );
    });
    int value = 0;
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop( value) );
    BOOST_CHECK_EQUAL( 1, value);
    f.join();
}

void test_push_closed() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    c.close();
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::closed == c.push( 1) );
}


void test_push_wait_for() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push_wait_for( 1, std::chrono::seconds( 1) ) );
    });
    int value = 0;
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop( value) );
    BOOST_CHECK_EQUAL( 1, value);
    f.join();
}

void test_push_wait_for_closed() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    c.close();
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::closed == c.push_wait_for( 1, std::chrono::seconds( 1) ) );
}

void test_push_wait_for_timeout() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c](){
        int value = 0;
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop( value) );
        BOOST_CHECK_EQUAL( 1, value);
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push_wait_for( 1, std::chrono::seconds( 1) ) );
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::timeout == c.push_wait_for( 1, std::chrono::seconds( 1) ) );
    f.join();
}

void test_push_wait_until() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push_wait_until( 1,
                        std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
    });
    int value = 0;
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop( value) );
    BOOST_CHECK_EQUAL( 1, value);
    f.join();
}

void test_push_wait_until_closed() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    c.close();
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::closed == c.push_wait_until( 1,
                    std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
}

void test_push_wait_until_timeout() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c](){
        int value = 0;
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop( value) );
        BOOST_CHECK_EQUAL( 1, value);
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push_wait_until( 1,
                std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::timeout == c.push_wait_until( 1,
                std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
    f.join();
}

void test_pop() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2, v2 = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&v1,&c](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop( v2) );
    BOOST_CHECK_EQUAL( v1, v2);
    f.join();
}

void test_pop_closed() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2, v2 = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&v1,&c](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
        c.close();
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop( v2) );
    BOOST_CHECK_EQUAL( v1, v2);
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::closed == c.pop( v2) );
    f.join();
}

void test_pop_success() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2, v2 = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c,&v2](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop( v2) );
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
    f.join();
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_value_pop() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2, v2 = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c,&v1](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
    });
    v2 = c.value_pop();
    f.join();
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_value_pop_closed() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c,&v1](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
        c.close();
    });
    int v2 = c.value_pop();
    BOOST_CHECK_EQUAL( v1, v2);
    f.join();
    bool thrown = false;
    try {
        c.value_pop();
    } catch ( memoria::v1::fibers::fiber_error const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_value_pop_success() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2, v2 = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c,&v2](){
        v2 = c.value_pop();
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
    f.join();
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_pop_wait_for() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2, v2 = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c,&v1](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop_wait_for( v2, std::chrono::seconds( 1) ) );
    f.join();
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_pop_wait_for_closed() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2, v2 = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c,&v1](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
        c.close();
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop_wait_for( v2, std::chrono::seconds( 1) ) );
    BOOST_CHECK_EQUAL( v1, v2);
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::closed == c.pop_wait_for( v2, std::chrono::seconds( 1) ) );
    f.join();
}

void test_pop_wait_for_success() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2, v2 = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c,&v2](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop_wait_for( v2, std::chrono::seconds( 1) ) );
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
    f.join();
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_pop_wait_for_timeout() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c,&v](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::timeout == c.pop_wait_for( v, std::chrono::seconds( 1) ) );
    });
    f.join();
}

void test_pop_wait_until() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2, v2 = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c,&v1](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop_wait_until( v2,
            std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
    BOOST_CHECK_EQUAL( v1, v2);
    f.join();
}

void test_pop_wait_until_closed() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2, v2 = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c,&v1](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
        c.close();
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop_wait_until( v2,
            std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
    BOOST_CHECK_EQUAL( v1, v2);
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::closed == c.pop_wait_until( v2,
            std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
    f.join();
}

void test_pop_wait_until_success() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v1 = 2, v2 = 0;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c,&v2](){
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop_wait_until( v2,
                    std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
    });
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( v1) );
    f.join();
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_pop_wait_until_timeout() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    int v = 0;
    BOOST_CHECK(
            memoria::v1::fibers::channel_op_status::timeout == c.pop_wait_until( v,
                std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
}

void test_wm_1() {
    memoria::v1::fibers::unbuffered_channel< int > c;
    std::vector< memoria::v1::fibers::fiber::id > ids;
    memoria::v1::fibers::fiber f1( memoria::v1::fibers::launch::dispatch, [&c,&ids](){
        ids.push_back( memoria::v1::this_fiber::get_id() );
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( 1) );

        ids.push_back( memoria::v1::this_fiber::get_id() );
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( 2) );

        ids.push_back( memoria::v1::this_fiber::get_id() );
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( 3) );

        ids.push_back( memoria::v1::this_fiber::get_id() );
        // would be blocked because channel is full
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( 4) );

        ids.push_back( memoria::v1::this_fiber::get_id() );
        // would be blocked because channel is full
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( 5) );

        ids.push_back( memoria::v1::this_fiber::get_id() );
    });
    memoria::v1::fibers::fiber f2( memoria::v1::fibers::launch::dispatch, [&c,&ids](){
        ids.push_back( memoria::v1::this_fiber::get_id() );
        BOOST_CHECK_EQUAL( 1, c.value_pop() );

        // let other fiber run
        memoria::v1::this_fiber::yield();

        ids.push_back( memoria::v1::this_fiber::get_id() );
        BOOST_CHECK_EQUAL( 2, c.value_pop() );

        ids.push_back( memoria::v1::this_fiber::get_id() );
        BOOST_CHECK_EQUAL( 3, c.value_pop() );

        ids.push_back( memoria::v1::this_fiber::get_id() );
        BOOST_CHECK_EQUAL( 4, c.value_pop() );

        ids.push_back( memoria::v1::this_fiber::get_id() );
        // would block because channel is empty
        BOOST_CHECK_EQUAL( 5, c.value_pop() );

        ids.push_back( memoria::v1::this_fiber::get_id() );
    });
    memoria::v1::fibers::fiber::id id1 = f1.get_id();
    memoria::v1::fibers::fiber::id id2 = f2.get_id();
    f1.join();
    f2.join();
    BOOST_CHECK_EQUAL( 12u, ids.size() );
    BOOST_CHECK_EQUAL( id1, ids[0]);
    BOOST_CHECK_EQUAL( id2, ids[1]);
    BOOST_CHECK_EQUAL( id1, ids[2]);
    BOOST_CHECK_EQUAL( id2, ids[3]);
    BOOST_CHECK_EQUAL( id2, ids[4]);
    BOOST_CHECK_EQUAL( id1, ids[5]);
    BOOST_CHECK_EQUAL( id2, ids[6]);
    BOOST_CHECK_EQUAL( id1, ids[7]);
    BOOST_CHECK_EQUAL( id2, ids[8]);
    BOOST_CHECK_EQUAL( id1, ids[9]);
    BOOST_CHECK_EQUAL( id2, ids[10]);
    BOOST_CHECK_EQUAL( id1, ids[11]);
}

void test_moveable() {
    memoria::v1::fibers::unbuffered_channel< moveable > c;
    memoria::v1::fibers::fiber f( memoria::v1::fibers::launch::dispatch, [&c]{
        moveable m1( 3);
        BOOST_CHECK( m1.state);
        BOOST_CHECK_EQUAL( 3, m1.value);
        BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.push( std::move( m1) ) );
    });
    moveable m2;
    BOOST_CHECK( ! m2.state);
    BOOST_CHECK_EQUAL( -1, m2.value);
    BOOST_CHECK( memoria::v1::fibers::channel_op_status::success == c.pop( m2) );
    BOOST_CHECK( m2.state);
    BOOST_CHECK_EQUAL( 3, m2.value);
    f.join();
}

void test_rangefor() {
    memoria::v1::fibers::unbuffered_channel< int > chan;
    std::vector< int > vec;
    memoria::v1::fibers::fiber f1( memoria::v1::fibers::launch::dispatch, [&chan]{
        chan.push( 1);
        chan.push( 1);
        chan.push( 2);
        chan.push( 3);
        chan.push( 5);
        chan.push( 8);
        chan.push( 12);
        chan.close();
    });
    memoria::v1::fibers::fiber f2( memoria::v1::fibers::launch::dispatch, [&vec,&chan]{
        for ( int value : chan) {
            vec.push_back( value);
        }
    });
    f1.join();
    f2.join();
    BOOST_CHECK_EQUAL( 1, vec[0]);
    BOOST_CHECK_EQUAL( 1, vec[1]);
    BOOST_CHECK_EQUAL( 2, vec[2]);
    BOOST_CHECK_EQUAL( 3, vec[3]);
    BOOST_CHECK_EQUAL( 5, vec[4]);
    BOOST_CHECK_EQUAL( 8, vec[5]);
    BOOST_CHECK_EQUAL( 12, vec[6]);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* []) {
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: unbuffered_channel test suite");

     test->add( BOOST_TEST_CASE( & test_push) );
     test->add( BOOST_TEST_CASE( & test_push_closed) );
     test->add( BOOST_TEST_CASE( & test_push_wait_for) );
     test->add( BOOST_TEST_CASE( & test_push_wait_for_closed) );
     test->add( BOOST_TEST_CASE( & test_push_wait_for_timeout) );
     test->add( BOOST_TEST_CASE( & test_push_wait_until) );
     test->add( BOOST_TEST_CASE( & test_push_wait_until_closed) );
     test->add( BOOST_TEST_CASE( & test_push_wait_until_timeout) );
     test->add( BOOST_TEST_CASE( & test_pop) );
     test->add( BOOST_TEST_CASE( & test_pop_closed) );
     test->add( BOOST_TEST_CASE( & test_pop_success) );
     test->add( BOOST_TEST_CASE( & test_value_pop) );
     test->add( BOOST_TEST_CASE( & test_value_pop_closed) );
     test->add( BOOST_TEST_CASE( & test_value_pop_success) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_for) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_for_closed) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_for_success) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_for_timeout) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_until) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_until_closed) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_until_success) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_until_timeout) );
     test->add( BOOST_TEST_CASE( & test_wm_1) );
     test->add( BOOST_TEST_CASE( & test_moveable) );
     test->add( BOOST_TEST_CASE( & test_rangefor) );

    return test;
}
