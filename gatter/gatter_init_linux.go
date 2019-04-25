package main

import (
	"github.com/go-ble/ble"
	"github.com/go-ble/ble/linux"
)

// DefaultDevice ...
func init() {
	dev, err := linux.NewDevice()
	if err != nil {
		panic(err)
	}
	ble.SetDefaultDevice(dev)
}
