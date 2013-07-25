/*

anerd-client-tcp: a Network Exchange Randomness Daemon web client

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
	"crypto/tls"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
)

type aNerdResponse struct {
	Size int
	Encoding string
	Data string
}

func main() {
	var err error
	/* Note: golang in Ubuntu 13.04 and earlier has a broken x509 certificate
	         loading mechanism
	*/
	tr := &http.Transport{
		TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
	}
	client := &http.Client{Transport: tr}
	resp, err := client.Get("https://anerd.us:26373")
	if err != nil {
		fmt.Println(err)
	} else {
		if body, err := ioutil.ReadAll(resp.Body); err == nil {
			var j aNerdResponse
			json.Unmarshal(body, &j)
			//fmt.Println(j.Data)
			data, _ := base64.StdEncoding.DecodeString(j.Data)
			f, _ := os.Create("/dev/urandom")
			f.Write(data)
			f.Close()
		}
	}
}
