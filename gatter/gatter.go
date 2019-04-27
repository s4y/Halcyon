package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"

	"github.com/godbus/dbus"
	ble "github.com/muka/go-bluetooth/api"
)

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

	char, err := dev.GetCharByUUID("0000ACED-0000-1000-8000-00805F9B34FB")
	chkErr(err)

	ch, err := char.Register()
	chkErr(err)

	err = char.StartNotify()
	chkErr(err)

	_, err = char.ReadValue(nil)
	chkErr(err)

	go func() {
		for {
			msg := <-ch
			body := msg.Body[1].(map[string]dbus.Variant)
			if val, ok := body["Value"]; ok {
				buf := val.Value().([]uint8)
				fmt.Printf("%02X%02X\n", buf[0], buf[1])
			} else {
				fmt.Printf("msg: %#v\n", msg)
			}
		}
	}()

	reader := bufio.NewReader(os.Stdin)
	for {
		text, _ := reader.ReadString('\n')
		if text == "" {
			break
		}
		text = strings.TrimRight(text, "\r\n")
		val, err := strconv.ParseUint(text, 16, 16)
		chkErr(err)

		err = char.WriteValue([]byte{
			byte(val >> 8),
			byte(val),
		}, nil)
		chkErr(err)
	}
}
