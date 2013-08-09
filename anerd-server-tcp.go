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
	"fmt"
	"io"
	"log/syslog"
	"net/http"
	"os"
	"time"
)

var log *syslog.Writer

const (
	DEFAULT_SIZE = 64
	DEVICE       = "/dev/urandom"
)

func handler(response http.ResponseWriter, request *http.Request) {
	checksum := sha512.New()
	io.WriteString(checksum, request.FormValue("challenge"))
	challenge_response := checksum.Sum(nil)
	io.WriteString(checksum, request.FormValue("uuid"))
	uuid := checksum.Sum(nil)
	log.Info(fmt.Sprintf("TCP Server received and hashed data from [%s, %s, %x] at [%v]", request.RemoteAddr, request.UserAgent(), uuid, time.Now().UnixNano()))
	dev, _ := os.Create(DEVICE)
	dev.WriteString(fmt.Sprintf("%d", time.Now().UnixNano()))
	dev.Write(uuid)
	dev.Write(challenge_response)
	dev.Close()
	data := make([]byte, DEFAULT_SIZE)
	io.ReadAtLeast(rand.Reader, data, DEFAULT_SIZE)
	io.WriteString(checksum, string(data[:DEFAULT_SIZE]))
	seed := checksum.Sum(nil)
	fmt.Fprintf(response, "%x\n%x\n", challenge_response, seed)
	log.Info(fmt.Sprintf("TCP Server sent hashed entropy to [%s, %s, %x] at [%v]", request.RemoteAddr, request.UserAgent(), uuid, time.Now().UnixNano()))
}

func main() {
	log, _ = syslog.New(syslog.LOG_ERR, "anerd")
	http.HandleFunc("/", handler)
	port := fmt.Sprintf(":%s", os.Args[1])
	http.ListenAndServeTLS(port, "/etc/anerd-server/cert.pem", "/etc/anerd-server/key.pem", nil)
}
