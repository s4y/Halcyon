package main

import (
	"context"
	"fmt"
	"log"
	"os"

	"github.com/go-ble/ble"
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

	client, err := ble.Connect(context.Background(), func(a ble.Advertisement) bool {
		return a.Addr().String() == addr
	})
	chkErr(err)

	profile, err := client.DiscoverProfile(false)
	chkErr(err)

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
