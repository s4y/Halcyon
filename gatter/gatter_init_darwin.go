package main

import (
	"github.com/go-ble/ble"
	"github.com/go-ble/ble/darwin"
)

// DefaultDevice ...
func init() {
	dev, err := darwin.NewDevice()
	if err != nil {
		panic(err)
	}
	ble.SetDefaultDevice(dev)
}
