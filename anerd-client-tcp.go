/*

anerd-client-tcp: a Network Exchange Randomness Daemon web client

Copyright 2013 Dustin Kirkland <dustin.kirkland@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

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
	resp, err := client.Get("https://localhost:26373")
	if err != nil {
		fmt.Println(err)
	} else {
		if body, err := ioutil.ReadAll(resp.Body); err == nil {
			var j aNerdResponse
			json.Unmarshal(body, &j)
			data, _ := base64.StdEncoding.DecodeString(j.Data)
			f, _ := os.Create("/dev/urandom")
			f.Write(data)
			f.Close()
		}
	}
}
