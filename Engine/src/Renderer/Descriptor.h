#pragma once

#include "pch.h"
#include "Device.h"

// for uniform buffer
class Descriptor {
private:
	Device* device;

public:
	Descriptor(Device* dev) : device(dev) {
	}

	~Descriptor() {
	}
};