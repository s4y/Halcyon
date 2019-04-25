package main

import (
	"context"
	"fmt"
	"log"
	"os"

	"github.com/go-ble/ble"
	"github.com/go-ble/ble/linux"
)

var wantService = ble.MustParse("acac")

var notifyCharacteristics = []ble.UUID{
	ble.MustParse("ac00"),
	ble.MustParse("ac20"),
	ble.MustParse("aced"),
}

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

	fmt.Printf("Alive?!?!?!\n")

	device, err := linux.NewDevice(ble.OptCentralRole())
	chkErr(err)
	ble.SetDefaultDevice(device)
	fmt.Printf("%#v\n", device)

	client, err := ble.Dial(context.Background(), ble.NewAddr(addr))
	// client, err := ble.Connect(context.Background(), func(a ble.Advertisement) bool {
	// 	for _, service := range a.Services() {
	// 		if service.Equal(wantService) {
	// 			return true
	// 		}
	// 		fmt.Printf("Non-matching service: %v\n", service)
	// 	}
	// 	return false
	// })
	chkErr(err)

	profile, err := client.DiscoverProfile(false)
	chkErr(err)

	fmt.Printf("%#v\n", profile)

	for _, want := range notifyCharacteristics {
		onChange := func(val []byte) {
			fmt.Printf("update for characteristic %v: %#v\n", want, val)
		}
		characteristic, _ := profile.Find(ble.NewCharacteristic(want)).(*ble.Characteristic)
		fmt.Printf("got %#v\n", characteristic)
		err = client.Subscribe(characteristic, false, onChange)
		chkErr(err)
		val, err := client.ReadCharacteristic(characteristic)
		chkErr(err)
		onChange(val)
	}
	select {}

	for _, service := range profile.Services {
		if service.UUID.Equal(wantService) {
			for _, characteristic := range service.Characteristics {
				for _, want := range notifyCharacteristics {
					if characteristic.UUID.Equal(want) {
						log.Println(client.ReadCharacteristic(characteristic))
						chkErr(client.Subscribe(characteristic, false, func(req []byte) {
							log.Println(characteristic.UUID, " changed to ", req)
						}))
					}
				}
			}
		}
	}

	<-client.Disconnected()
}
