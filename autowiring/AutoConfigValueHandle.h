// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Autowired.h"
#include "AutoConfigManager.h"

#include <string>

template<typename T>
class AutoConfigValueHandle {
  const std::string& m_key;
  std::shared_ptr<AutoConfigManager> m_manager;
  T& m_value;
  bool m_dirty = false;

  AutoConfigValueHandle(std::shared_ptr<AutoConfigManager> manager, const std::string& key) :
    m_key(key), m_manager(manager), m_value(*m_manager->Get(m_key)->template as<T>().get())
  {
    
  }

  template<class U, class ...t_Key>
  friend class AutoConfig;
public:
  ~AutoConfigValueHandle() {
    if (m_dirty)
      m_manager->Set(m_key, m_value); //triggers callbacks & recursive setting
  }

  const T* operator->(void) const {
    return m_value;
  }
  T* operator->(void) {
    m_dirty = true;
    return &m_value;
  }
  const T& operator*() const {
    return m_value;
  }
  T& operator*() {
    m_dirty = true;
    return m_value;
  }
};