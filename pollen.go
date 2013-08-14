/*

pollen: Entropy-as-a-Server web server

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

var dev *os.File

const (
	DEFAULT_SIZE = 64
	DEVICE       = "/dev/urandom"
)

func handler(response http.ResponseWriter, request *http.Request) {
	dev.WriteString(fmt.Sprintf("%d", time.Now().UnixNano()))
	checksum := sha512.New()
	io.WriteString(checksum, request.FormValue("challenge"))
	dev.WriteString(fmt.Sprintf("%d", time.Now().UnixNano()))
	challenge_response := checksum.Sum(nil)
	dev.WriteString(fmt.Sprintf("%d", time.Now().UnixNano()))
	dev.Write(challenge_response)
	io.WriteString(checksum, request.FormValue("tag"))
	tag := checksum.Sum(nil)
	dev.WriteString(fmt.Sprintf("%d", time.Now().UnixNano()))
	dev.Write(tag)
	log.Info(fmt.Sprintf("Server received challenge from [%s, %s, %x] at [%v]", request.RemoteAddr, request.UserAgent(), tag, time.Now().UnixNano()))
	data := make([]byte, DEFAULT_SIZE)
	io.ReadAtLeast(rand.Reader, data, DEFAULT_SIZE)
	io.WriteString(checksum, string(data[:DEFAULT_SIZE]))
	seed := checksum.Sum(nil)
	dev.WriteString(fmt.Sprintf("%d", time.Now().UnixNano()))
	fmt.Fprintf(response, "%x\n%x\n", challenge_response, seed)
	log.Info(fmt.Sprintf("Server sent response to [%s, %s, %x] at [%v]", request.RemoteAddr, request.UserAgent(), tag, time.Now().UnixNano()))
	dev.WriteString(fmt.Sprintf("%d", time.Now().UnixNano()))
}

func main() {
	log, _ = syslog.New(syslog.LOG_ERR, "pollen")
	dev, _ = os.Create(DEVICE)
	http.HandleFunc("/", handler)
	port := fmt.Sprintf(":%s", os.Args[1])
	http.ListenAndServeTLS(port, "/etc/pollen/cert.pem", "/etc/pollen/key.pem", nil)
	dev.Close()
}
