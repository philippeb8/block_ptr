/**
    @file
    Boost block_ptr.hpp header file.

    @author
    Copyright (c) 2008 Phil Bouchard <pbouchard8@gmail.com>.

    @note
    Distributed under the Boost Software License, Version 1.0.

    See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt

    See http://www.boost.org/libs/smart_ptr/doc/index.html for documentation.


    Thanks to: Steven Watanabe <watanabesj@gmail.com>
*/


#ifndef BOOST_BLOCK_PTR_INCLUDED
#define BOOST_BLOCK_PTR_INCLUDED


#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4355 )

#include <new.h>
#endif

#ifndef BOOST_DISABLE_THREADS
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#endif

#include <boost/smart_ptr/detail/intrusive_list.hpp>
#include <boost/smart_ptr/detail/intrusive_stack.hpp>
#include <boost/smart_ptr/detail/classof.hpp>
#include <boost/smart_ptr/detail/block_ptr_base.hpp>
#include <boost/smart_ptr/detail/system_pool.hpp>


namespace boost
{

namespace smart_ptr
{

namespace detail
{


struct block_base;


} // namespace detail

} // namespace smart_ptr

/**
    Set header.
    
    Proxy object used to count the number of pointers from the stack are referencing pointee objects belonging to the same @c block_proxy .
*/

struct block_proxy
{
    long count_;                                    /**< Count of the number of pointers from the stack referencing the same @c block_proxy .*/
    bool destroying_;                                   /**< Destruction sequence initiated. */
    smart_ptr::detail::intrusive_list::node proxy_tag_;                /**< Tag used to enlist to @c block_proxy::includes_ . */
    smart_ptr::detail::intrusive_list block_list_;                     /**< List of all pointee objects belonging to a @c block_proxy . */

#ifndef BOOST_DISABLE_THREADS
    static mutex & static_mutex()                   /**< Main global mutex used for thread safety */
    {
        static mutex mutex_;
        
        return mutex_;
    }
#endif

    /**
        Initialization of a single @c block_proxy .
    */
    
    block_proxy() : count_(0), destroying_(false)
    {
    }
    
    
    ~block_proxy()
    {
        release();
    }
    
    
    bool destroying() const
    {
        return destroying_;
    }

    
    void destroying(bool b)
    {
        destroying_ = b;
    }

    
    /**
        Enlist & initialize pointee objects belonging to the same @c block_proxy .  This initialization occurs when a pointee object is affected to the first pointer living on the stack it encounters.
        
        @param  p   Pointee object to initialize.
    */
    
    void init(smart_ptr::detail::block_base * p)
    {
        block_list_.push_back(& p->block_tag_);
    }
    
    
    void release()
    {
        using namespace smart_ptr::detail;
        
        destroying(true);

        for (intrusive_list::iterator<block_base, &block_base::block_tag_> m = block_list_.begin(), n = block_list_.begin(); m != block_list_.end(); m = n)
        {
            ++ n;
            delete &* m;
        }
        
        destroying(false);
    }
};


#define TEMPLATE_DECL(z, n, text) BOOST_PP_COMMA_IF(n) typename T ## n
#define ARGUMENT_DECL(z, n, text) BOOST_PP_COMMA_IF(n) T ## n const & t ## n
#define PARAMETER_DECL(z, n, text) BOOST_PP_COMMA_IF(n) t ## n

#define BEFRIEND_MAKE_BLOCK(z, n, text)																			    	\
    template <typename V, BOOST_PP_REPEAT(n, TEMPLATE_DECL, 0)>										                    \
        friend block_ptr<V, UserPool> text(BOOST_PP_REPEAT(n, ARGUMENT_DECL, 0));

#define CONSTRUCT_MAKE_BLOCK1(z, n, text)																			    \
    template <typename V, BOOST_PP_REPEAT(n, TEMPLATE_DECL, 0), typename UserPool = smart_ptr::detail::system_pool<smart_ptr::detail::system_pool_tag, sizeof(char)> >										                    \
        block_ptr<V, UserPool> text(BOOST_PP_REPEAT(n, ARGUMENT_DECL, 0))															\
        {																												\
            return block_ptr<V, UserPool>(new block<V, UserPool>(BOOST_PP_REPEAT(n, PARAMETER_DECL, 0)));									\
        }

#define CONSTRUCT_MAKE_BLOCK2(z, n, text)                                                                                \
    template <typename V, BOOST_PP_REPEAT(n, TEMPLATE_DECL, 0), typename UserPool = smart_ptr::detail::system_pool<smart_ptr::detail::system_pool_tag, sizeof(char)> >                                                          \
        block_ptr<V, UserPool> text(smart_ptr::detail::block_ptr_base<smart_ptr::detail::block_proxy, UserPool> & q, BOOST_PP_REPEAT(n, ARGUMENT_DECL, 0))                                                           \
        {                                                                                                               \
            return block_ptr<V, UserPool>(q, new block<V, UserPool>(BOOST_PP_REPEAT(n, PARAMETER_DECL, 0)));                                   \
        }

#define CONSTRUCT_MAKE_BLOCK3(z, n, text)                                                                               \
    template <typename V, BOOST_PP_REPEAT(n, TEMPLATE_DECL, 0), typename UserPool = smart_ptr::detail::system_pool<smart_ptr::detail::system_pool_tag, sizeof(char)> >                                                          \
        block_ptr<V, UserPool> text(BOOST_PP_REPEAT(n, ARGUMENT_DECL, 0))                                                           \
        {                                                                                                               \
            return block_ptr<V, UserPool>(new fastblock<V, UserPool>(BOOST_PP_REPEAT(n, PARAMETER_DECL, 0)));                                   \
        }

#define CONSTRUCT_MAKE_BLOCK4(z, n, text)                                                                                \
    template <typename V, BOOST_PP_REPEAT(n, TEMPLATE_DECL, 0), typename UserPool = smart_ptr::detail::system_pool<smart_ptr::detail::system_pool_tag, sizeof(char)> >                                                          \
        block_ptr<V, UserPool> text(smart_ptr::detail::block_ptr_base<smart_ptr::detail::block_proxy, UserPool> & q, BOOST_PP_REPEAT(n, ARGUMENT_DECL, 0))                                                           \
        {                                                                                                               \
            return block_ptr<V, UserPool>(q, new fastblock<V, UserPool>(BOOST_PP_REPEAT(n, PARAMETER_DECL, 0)));                                   \
        }


/**
    Deterministic region based memory manager.
    
    Complete memory management utility on top of standard reference counting.
*/

template <typename T, typename UserPool = smart_ptr::detail::system_pool<smart_ptr::detail::system_pool_tag, sizeof(char)> >
    class block_ptr : public smart_ptr::detail::block_ptr_base<T, UserPool>
    {
        template <typename, typename> friend class block_ptr;

        typedef smart_ptr::detail::block_ptr_base<T, UserPool> base;
        
        using base::share;
        using base::po_;

        block_proxy const & x_;                      /**< Pointer to the @c block_proxy node @c block_ptr<> belongs to. */
        
    public:
        /**
            Initialization of a pointer.
            
            @param  p   New pointee object to manage.
        */
        
        explicit block_ptr(block_proxy const & x) : base(), x_(x)
        {
        }

            
        /**
            Initialization of a pointer.
            
            @param	p	New pointee object to manage.
        */
        
        template <typename V>
            explicit block_ptr(block_proxy const & x, block<V, UserPool> * p) : base(p), x_(x)
            {
                const_cast<block_proxy &>(x_).init(p);
            }

        
    public:
        typedef T                           value_type;


        /**
            Initialization of a pointer.
            
            @param	p	New pointer to manage.
        */

        template <typename V>
            block_ptr(block_ptr<V, UserPool> const & p) : base(p), x_(p.x_)
            {
#ifndef BOOST_DISABLE_THREADS
                mutex::scoped_lock scoped_lock(block_proxy::static_mutex());
#endif
            }

        
        /**
            Initialization of a pointer.
            
            @param	p	New pointer to manage.
        */

            block_ptr(block_ptr<T, UserPool> const & p) : base(p), x_(p.x_)
            {
#ifndef BOOST_DISABLE_THREADS
                mutex::scoped_lock scoped_lock(block_proxy::static_mutex());
#endif
            }


        /**
            Assignment.
            
            @param	p	New pointer to manage.
        */
            
        template <typename V>
            block_ptr & operator = (block_ptr<V, UserPool> const & p)
            {
#ifndef BOOST_DISABLE_THREADS
                mutex::scoped_lock scoped_lock(block_proxy::static_mutex());
#endif

                base::operator = (p);

                return * this;
            }


        /**
            Assignment.
            
            @param	p	New pointer to manage.
        */

        block_ptr & operator = (block_ptr<T, UserPool> const & p)
        {
            return operator = <T>(p);
        }

        void reset()
        {
#ifndef BOOST_DISABLE_THREADS
            mutex::scoped_lock scoped_lock(block_proxy::static_mutex());
#endif

            release();
        }
        
        /**
            Assignment.
            
            @param  p   New pointer to manage.
        */

        template <typename V>
            block_ptr & operator = (block<V, UserPool> * p)
            {
                const_cast<block_proxy &>(x_).init(p);

                base::operator = (p);
                
                return * this;
            }

        template <typename V>
            void reset(block_ptr<V, UserPool> const & p)
            {
                operator = <T>(p);
            }
        
        template <typename V>
            void reset(block<V, UserPool> * p)
            {
                operator = <T>(p);
            }
        
        bool cyclic() const
        {
            return x_.destroying();
        }

        ~block_ptr()
        {
            if (cyclic())
                base::po_ = 0;
            else
                release();
        }

    private:
        /**
            Release of the pointee object with or without destroying the entire @c block_proxy it belongs to.
        */
        
        void release()
        {
            base::reset();
        }

        
#if 0 //defined(BOOST_HAS_RVALUE_REFS)
    public:
        block_ptr(block_ptr<T> && p): base(p.po_), x_(p.x_)
        {
            p.po_ = 0;
        }

        template<class Y>
            block_ptr(block_ptr<Y> && p): base(p.po_), x_(p.x_)
            {
                p.po_ = 0;
            }

        block_ptr<T> & operator = (block_ptr<T> && p)
        {
            std::swap(po_, p.po_);
            std::swap(x_, p.x_);
            
            return *this;
        }

        template<class Y>
            block_ptr & operator = (block_ptr<Y> && p)
            {
                std::swap(po_, p.po_);
                std::swap(x_, p.x_);
                
                return *this;
            }
#endif
    };


/**
    Helper.
*/
    
template <typename T, typename UserPool = smart_ptr::detail::system_pool<smart_ptr::detail::system_pool_tag, sizeof(char)> >
    struct root_ptr : block_proxy, block_ptr<T, UserPool>
    {
        root_ptr() : block_proxy(), block_ptr<T, UserPool>(* static_cast<block_proxy *>(this)) 
        {
        }
        
        
        /**
            Initialization of a pointer.
            
            @param  p   New pointee object to manage.
        */
        
        template <typename V>
            explicit root_ptr(block<V, UserPool> * p) : block_ptr<T, UserPool>(*this, p)
            {
            }
            
            
        block_ptr<T, UserPool> & operator = (block_ptr<T, UserPool> const & p)
        {
            return block_ptr<T, UserPool>::operator = (p);
        }
    };


template <typename V, typename UserPool = smart_ptr::detail::system_pool<smart_ptr::detail::system_pool_tag, sizeof(char)> >
    block_ptr<V, UserPool> make_block()
    {
        return block_ptr<V, UserPool>(new block<V, UserPool>());
    }

template <typename V, typename UserPool = smart_ptr::detail::system_pool<smart_ptr::detail::system_pool_tag, sizeof(char)> >
    block_ptr<V, UserPool> make_block(smart_ptr::detail::block_ptr_base<smart_ptr::detail::block_proxy, UserPool> & q)
    {
        return block_ptr<V, UserPool>(q, new block<V, UserPool>());
    }

template <typename V, typename UserPool = smart_ptr::detail::system_pool<smart_ptr::detail::system_pool_tag, sizeof(char)> >
    block_ptr<V, UserPool> make_fastblock()
    {
        return block_ptr<V, UserPool>(new fastblock<V, UserPool>());
    }

template <typename V, typename UserPool = smart_ptr::detail::system_pool<smart_ptr::detail::system_pool_tag, sizeof(char)> >
    block_ptr<V, UserPool> make_fastblock(smart_ptr::detail::block_ptr_base<smart_ptr::detail::block_proxy, UserPool> & q)
    {
        return block_ptr<V, UserPool>(q, new fastblock<V, UserPool>());
    }

template <typename T, typename UserPool>
    bool operator == (block_ptr<T, UserPool> const &a1, block_ptr<T, UserPool> const &a2)
    {
        return a1.get() == a2.get();
    }

template <typename T, typename UserPool>
    bool operator != (block_ptr<T, UserPool> const &a1, block_ptr<T, UserPool> const &a2)
    {
        return a1.get() != a2.get();
    }


BOOST_PP_REPEAT_FROM_TO(1, 10, CONSTRUCT_MAKE_BLOCK1, make_block)
BOOST_PP_REPEAT_FROM_TO(1, 10, CONSTRUCT_MAKE_BLOCK2, make_block)
BOOST_PP_REPEAT_FROM_TO(1, 10, CONSTRUCT_MAKE_BLOCK3, make_fastblock)
BOOST_PP_REPEAT_FROM_TO(1, 10, CONSTRUCT_MAKE_BLOCK4, make_fastblock)

} // namespace boost


#if defined(_MSC_VER)
#pragma warning( pop )
#endif


#endif // #ifndef BOOST_BLOCK_PTR_INCLUDED
