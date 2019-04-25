package main

import (
	"fmt"
	"os"

	"github.com/godbus/dbus"
	ble "github.com/muka/go-bluetooth/api"
)

// var wantService = ble.MustParse("acac")
//
// var notifyCharacteristics = []ble.UUID{
// 	ble.MustParse("ac00"),
// 	ble.MustParse("ac20"),
// 	ble.MustParse("aced"),
// }

func chkErr(err error) {
	if err != nil {
		panic(err)
	}
}

func main() {
	if len(os.Args) != 2 {
		fmt.Fprintf(os.Stderr, "usage: %s peripehral_address\n", os.Args[0])
		os.Exit(1)
	}
	addr := os.Args[1]

	dev, err := ble.GetDeviceByAddress(addr)
	chkErr(err)

	err = dev.Connect()
	chkErr(err)

	// fmt.Printf("Got device %v\n", dev)

	// b00fchar, err := dev.GetCharByUUID("0000B00F-0000-1000-8000-00805F9B34FB")
	// chkErr(err)

	// b00fch, err := b00fchar.Register()
	// go func() {
	// 	for {
	// 		msg := <-b00fch

	// 		if msg == nil {
	// 			return
	// 		}

	// 		fmt.Printf("b00f Message %v\n", msg)
	// 	}
	// }()

	// b00fchar.StartNotify()

	char, err := dev.GetCharByUUID("0000ACED-0000-1000-8000-00805F9B34FB")
	chkErr(err)
	// fmt.Printf("Got char %v\n", char)

	ch, err := char.Register()
	chkErr(err)

	err = char.StartNotify()
	chkErr(err)

	_, err = char.ReadValue(nil)
	chkErr(err)
	// fmt.Printf("Got value %v\n", val)

	for {
		msg := <-ch

		if msg == nil {
			return
		}

		body := msg.Body[1].(map[string]dbus.Variant)

		if val, ok := body["Value"]; ok {
			buf := val.Value().([]uint8)
			fmt.Printf("Change to %02X%02X\n", buf[0], buf[1])
		}
	}
}
