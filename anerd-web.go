/*

anerd-web: Asynchronous Network Exchange Randomness Daemon Web Server

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
	"bytes"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"crypto/rand"
	"net/http"
)

var DEFAULT_SIZE int64 = 64

type aNerdResponse struct {
	Size int64
	Encoding string
	Data string
}

func handler(w http.ResponseWriter, r *http.Request) {
	data := make([]byte, DEFAULT_SIZE)
	rand.Read(data)
	buf := &bytes.Buffer{}
	encoder := base64.NewEncoder(base64.StdEncoding, buf);
	encoder.Write([]byte(data));
	encoder.Close()
	a := aNerdResponse{Size:DEFAULT_SIZE, Encoding:"base64", Data:buf.String()}
	j, _ := json.Marshal(a)
	fmt.Fprintf(w, "%s", j)
}

func main() {
	http.HandleFunc("/", handler)
	http.ListenAndServe(":8080", nil)
}
