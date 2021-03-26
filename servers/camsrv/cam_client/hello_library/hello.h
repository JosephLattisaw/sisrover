// Copyright (c) 2019, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "stdint.h"

void hello_world();

int32_t foo(
            int32_t bar, 
            int32_t (*callback)(void*, int32_t)
            );
