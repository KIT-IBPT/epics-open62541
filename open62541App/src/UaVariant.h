/*
 * Copyright 2019 aquenos GmbH.
 * Copyright 2019 Karlsruhe Institute of Technology.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * This software has been developed by aquenos GmbH on behalf of the
 * Karlsruhe Institute of Technology's Institute for Beam Physics and
 * Technology.
 *
 * This software contains code originally developed by aquenos GmbH for
 * the s7nodave EPICS device support. aquenos GmbH has relicensed the
 * affected portions of code from the s7nodave EPICS device support
 * (originally licensed under the terms of the GNU GPL) under the terms
 * of the GNU LGPL version 3 or newer.
 */

#ifndef OPEN62541_EPICS_UA_VARIANT_H
#define OPEN62541_EPICS_UA_VARIANT_H

#include <cstdint>
#include <functional>
#include <string>

extern "C" {
#include "open62541.h"
}

#include "UaException.h"

namespace open62541 {
namespace epics {

/**
 * OPC UA value variant. This is a wrapper around UA_Variant that takes care of
 * ensuring that memory is correctly handled when creating, copying and deleting
 * variants. A variant represents a value with a type that is only determined at
 * runtime.
 */
class UaVariant {

  // TODO Use UaVariant everywhere in our code where we currently use UA_Variant.

public:

  /**
   * Creates an empty variant. Such an object does not represent a value, but it
   * can safely be assigned or deleted.
   */
  inline UaVariant() {
    UA_Variant_init(&this->value);
  }

  /**
   * Creates a variant that is a copy of the passed variant.
   */
  UaVariant(UaVariant const &other);

  /**
   * Creates a variant moving the data from another variant.
   */
  inline UaVariant(UaVariant &&other) {
    this->value = other.value;
    UA_Variant_init(&other.value);
  }

  /**
   * Creates a variant that is a copy of the passed variant.
   */
  UaVariant(UA_Variant const &value);

  /**
   * Creates a variant moving the data from another variant.
   */
  UaVariant(UA_Variant &&value) {
    this->value = value;
    UA_Variant_init(&value);
  }

  /**
   * Destructor.
   */
  inline ~UaVariant() {
    UA_Variant_clear(&this->value);
  }

  UaVariant &operator=(UaVariant const &other);

  inline UaVariant &operator=(UaVariant &&other) {
    UA_Variant_clear(&this->value);
    this->value = other.value;
    UA_Variant_init(&other.value);
    return *this;
  }

  inline explicit operator bool() const {
    return !UA_Variant_isEmpty(&this->value);
  }

  /**
   * Returns a reference to the underlying variant as used by the open62541
   * library.
   */
  inline UA_Variant const & get() const {
    return value;
  }

private:

  UA_Variant value;

};

} // namespace epics
} // namespace open62541

#endif // OPEN62541_EPICS_UA_VARIANT_H
