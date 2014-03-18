/*
 * CPU Affinity
 * Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2013 Japan Science and Technology Agency
 * $Id: CPUAffinity.hpp 450 2013-07-29 01:49:19Z iwashita $
 */

#pragma once

#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <ostream>

#include "MyVector.hpp"

namespace tdzdd {

class CPUAffinity {
    int const m;
    int const n;
    MyVector<size_t> idle;
    MyVector<int> cpu;

    void initialize() {
        // /proc/stat を読む
    }

public:
    CPUAffinity()
            : m(sysconf(_SC_NPROCESSORS_CONF)), n(m), idle(m), cpu(n) {
        initialize();
    }

    CPUAffinity(int n)
            : m(sysconf(_SC_NPROCESSORS_CONF)), n(n), idle(m), cpu(n) {
        initialize();
    }

    void bind(int k) const {
        cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET(k % n, &mask);

        if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
            std::cerr << "WARNING: Failed to set CPU affinity\n";
        }
    }
};

}
