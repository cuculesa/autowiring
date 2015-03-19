// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "AutoConfigParser.hpp"
#include "demangle.h"
#include "expect.hpp"
#include <sstream>
  
std::string autowiring::ExtractKeyUnix(const std::string& demangled) {
  //Extract Namespace and value from typename
  //ConfigTypeExtractor<Namespace, Value>
  std::stringstream ss(demangled);
  
  std::string arg1;
  std::string arg2;
  ss >> expect("ConfigTypeExtractor<");
  ss >> arg1;
  
  // If arg1 contains a comma, there are 2 arguments
  auto found = arg1.find(",");
  if (found != std::string::npos) {
    ss >> arg2;
    
    // Remove trailing ","
    arg1.resize(arg1.size()-1);
    
    // Remove trailing '>'
    arg2.resize(arg2.size()-1);
    
    std::stringstream key;
    key << arg1 << "." << arg2;
    return key.str();
    
  } else {
    // Remove trailing '>'
    arg1.resize(arg1.size()-1);
    return arg1;
  }
}

std::string autowiring::ExtractKeyWin(const std::string& demangled) {
  //Extract Namespace and value from typename
  //struct ConfigTypeExtractor<struct Namespace, struct ..., struct Value>

  const auto identifiersStart = demangled.find('<');
  const auto identifiersEnd = demangled.rfind('>');
  if (identifiersStart == (std::string::npos) || identifiersEnd == std::string::npos)
    return std::string();

  std::string key;
  
  auto subKeyStart = identifiersStart + 1;
  auto subKeyEnd = subKeyStart;

  while (subKeyStart < identifiersEnd) {
    //Find the > or , which denotes the start of the next identifier (or the end of the identifiers)
    subKeyEnd = demangled.find_first_of(",>", subKeyStart);

    //Find the end of :: or the space between the struct declaration and our name backwards from the end of the identifer
    subKeyStart = demangled.find_last_of(": ", subKeyEnd);

    key += demangled.substr(subKeyStart + 1, (subKeyEnd-subKeyStart)-1);
    key += ".";

    subKeyStart = subKeyEnd + 1;
  }

  key.resize(key.size() - 1); //trim the trailing .
  return key;

  /*
  // If arg1 contains a comma, there are 2 arguments
  auto found = arg1.find(",struct");
  if (found != std::string::npos) {
    ss >> arg2;
    
    // Remove trailing ",struct"
    arg1 = arg1.substr(0, found);
    
    // Remove trailing '>'
    arg2.resize(arg2.size()-1);
    
    std::stringstream key;
    key << arg1 << "." << arg2;
    return key.str();
    
  } else {
    // Remove trailing '>'
    arg1.resize(arg1.size()-1);
    return arg1;
  }*/
}

std::string autowiring::ExtractKey(const std::type_info& ti) {
  std::string demangled = demangle(ti);

  return
#if __GNUG__// Mac and linux
  ExtractKeyUnix(demangled);
#else // Windows
  ExtractKeyWin(demangled);
#endif
}
