/*

anerd-server-tcp: a Network Exchange Randomness Daemon Web Server

  Copyright (C) 2012-2013 Dustin Kirkland <dustin.kirkland@gmail.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, version 3 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

package main

import (
	"crypto/rand"
	"crypto/sha512"
	"encoding/json"
	"fmt"
	"io"
	"log/syslog"
	"net/http"
	"os"
	"time"
)

const (
	DEFAULT_SIZE = 64
	DEVICE       = "/dev/urandom"
)

type aNerdResponse struct {
	Format string
	Data   string
}

func handler(response http.ResponseWriter, request *http.Request) {
	log, _ := syslog.New(syslog.LOG_ERR, "anerd")
	checksum := sha512.New()
	io.WriteString(checksum, request.FormValue("uuid"))
	uuid := checksum.Sum(nil)
	io.WriteString(checksum, request.FormValue("tip"))
	tip := checksum.Sum(nil)
	log.Info(fmt.Sprintf("TCP Server received and hashed data from [%s, %x]", request.RemoteAddr, uuid))
	dev, _ := os.Create(DEVICE)
	dev.WriteString(fmt.Sprintf("%d", time.Now().UnixNano()))
	dev.Write(uuid)
	dev.Write(tip)
	dev.Close()
	data := make([]byte, DEFAULT_SIZE)
	io.ReadAtLeast(rand.Reader, data, DEFAULT_SIZE)
	io.WriteString(checksum, string(data[:DEFAULT_SIZE]))
	hash := checksum.Sum(nil)
	anerd := aNerdResponse{Format: "sha512", Data: fmt.Sprintf("%x", hash)}
	data, err := json.MarshalIndent(anerd, "", "    ")
	if err == nil {
		fmt.Fprintf(response, "%s", data)
		log.Info(fmt.Sprintf("TCP Server sent hashed entropy to [%s, %x]", request.RemoteAddr, uuid))
	}
}

func main() {
	http.HandleFunc("/", handler)
	port := fmt.Sprintf(":%s", os.Args[1])
	http.ListenAndServeTLS(port, "/etc/anerd-server/cert.pem", "/etc/anerd-server/key.pem", nil)
}
