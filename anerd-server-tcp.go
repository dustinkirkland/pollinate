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
	"bytes"
	"crypto/rand"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"log/syslog"
	"net/http"
	"os"
)

var DEFAULT_SIZE int = 64

type aNerdResponse struct {
	Size     int
	Encoding string
	Data     string
}

func handler(w http.ResponseWriter, r *http.Request) {
	data := make([]byte, DEFAULT_SIZE)
	log, _ := syslog.New(syslog.LOG_ERR, "anerd")
	uuid := r.FormValue("uuid")
	tip := r.FormValue("tip")
	log.Info(fmt.Sprintf("TCP Server recv [%d bytes] from [%s, %s]", len(tip), r.RemoteAddr, uuid))
	for {
		n, err := io.ReadAtLeast(rand.Reader, data, DEFAULT_SIZE)
		if err == nil || n == DEFAULT_SIZE {
			break
		}

	}
	buf := &bytes.Buffer{}
	encoder := base64.NewEncoder(base64.StdEncoding, buf)
	encoder.Write([]byte(data))
	encoder.Close()
	a := aNerdResponse{Size: DEFAULT_SIZE, Encoding: "base64", Data: buf.String()}
	j, err := json.MarshalIndent(a, "", "    ")
	if err == nil {
		fmt.Fprintf(w, "%s", j)
		log.Info(fmt.Sprintf("TCP Server sent [%d bytes] to [%s, %s]", DEFAULT_SIZE, r.RemoteAddr, uuid))
	}
}

func main() {
	http.HandleFunc("/", handler)
	port := fmt.Sprintf(":%s", os.Args[1])
	http.ListenAndServeTLS(port, "/etc/anerd-server/cert.pem", "/etc/anerd-server/key.pem", nil)
}
