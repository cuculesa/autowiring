/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2007-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef AUTOBOOST_INTRUSIVE_TREE_NODE_HPP
#define AUTOBOOST_INTRUSIVE_TREE_NODE_HPP

#if defined(_MSC_VER)
#  pragma once
#endif

#include <autoboost/intrusive/detail/config_begin.hpp>
#include <autoboost/intrusive/pointer_rebind.hpp>

namespace autoboost {
namespace intrusive {

template<class VoidPointer>
struct tree_node
{
   typedef typename pointer_rebind<VoidPointer, tree_node>::type  node_ptr;

   node_ptr parent_, left_, right_;
};

template<class VoidPointer>
struct tree_node_traits
{
   typedef tree_node<VoidPointer> node;

   typedef typename node::node_ptr   node_ptr;
   typedef typename pointer_rebind<VoidPointer, const node>::type const_node_ptr;

   static node_ptr get_parent(const const_node_ptr & n)
   {  return n->parent_;  }

   static node_ptr get_parent(const node_ptr & n)
   {  return n->parent_;  }

   static void set_parent(const node_ptr & n, const node_ptr & p)
   {  n->parent_ = p;  }

   static node_ptr get_left(const const_node_ptr & n)
   {  return n->left_;  }

   static node_ptr get_left(const node_ptr & n)
   {  return n->left_;  }

   static void set_left(const node_ptr & n, const node_ptr & l)
   {  n->left_ = l;  }

   static node_ptr get_right(const const_node_ptr & n)
   {  return n->right_;  }

   static node_ptr get_right(const node_ptr & n)
   {  return n->right_;  }

   static void set_right(const node_ptr & n, const node_ptr & r)
   {  n->right_ = r;  }
};

} //namespace intrusive
} //namespace autoboost

#include <autoboost/intrusive/detail/config_end.hpp>

#endif //AUTOBOOST_INTRUSIVE_TREE_NODE_HPP
