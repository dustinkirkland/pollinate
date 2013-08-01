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

var DEFAULT_SIZE int = 64

var DEVICE string = "/dev/urandom"

type aNerdResponse struct {
	Format string
	Data   string
}

func handler(w http.ResponseWriter, r *http.Request) {
	log, _ := syslog.New(syslog.LOG_ERR, "anerd")
	h := sha512.New()
	io.WriteString(h, r.FormValue("uuid"))
	uuid := h.Sum(nil)
	io.WriteString(h, r.FormValue("tip"))
	tip := h.Sum(nil)
	log.Info(fmt.Sprintf("TCP Server received and hashed data from [%s, %x]", r.RemoteAddr, uuid))
	f, _ := os.Create(DEVICE)
	f.WriteString(fmt.Sprintf("%d", time.Now().UnixNano()))
	f.Write(uuid)
	f.Write(tip)
	f.Close()
	data := make([]byte, DEFAULT_SIZE)
	io.ReadAtLeast(rand.Reader, data, DEFAULT_SIZE)
	io.WriteString(h, string(data[:DEFAULT_SIZE]))
	hash := h.Sum(nil)
	a := aNerdResponse{Format: "sha512", Data: fmt.Sprintf("%x", hash)}
	j, err := json.MarshalIndent(a, "", "    ")
	if err == nil {
		fmt.Fprintf(w, "%s", j)
		log.Info(fmt.Sprintf("TCP Server sent hashed entropy to [%s, %x]", r.RemoteAddr, uuid))
	}
}

func main() {
	http.HandleFunc("/", handler)
	port := fmt.Sprintf(":%s", os.Args[1])
	http.ListenAndServeTLS(port, "/etc/anerd-server/cert.pem", "/etc/anerd-server/key.pem", nil)
}
