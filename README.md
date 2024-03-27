# cgi-programming-training

### 要求

1. 透過 Mailefile 來編譯 x86_64, arm_64, arm_al 三個平台的可執行檔
2. 使用 C/C++ 建立 1 個 CGI 程式，可置於 /home/htttpd/cgi-bin/ 底下運作
3. 承 2 項目 CGI 程式需要可以擷取 QUERY STRING 參數執行以下功能
   1. 實現目錄列舉 (fn=ls) 功能，並依據 (file=) 參數作為指定路徑(限定/share/底下)進行列舉，完成列舉的回應內容必須以 utf8 plain text 格式逐檔案印出以下內容：權限(8 進位) 檔案大小 檔名<換行>
   2. 實現程式 CPU(percentage)/Memory(bytes) 使用率(fn=usage)的探測功能，並且接受 (pid=) 參數作為目標程式(不給值代表預設為自己)來進行幾秒(s=)間的使用率計算，完成列舉的回應內容必須以 utf8 json 格式印出以下內容： {"comm":"find","cpu":23.4,"memory":10240}
   3. 找出開啟目標檔案的程式(fn=lsof)，接受 (file=) 作為參數，將所有開啟該檔案的程式 (comm) 逐一使用 utf8 plain text 列舉出來

### 編譯指令

一次到位

```console
gcc -g main.c cJSON.c add.c -o ./build/main.cgi -lpthread
gcc -g main.c cJSON.c add.c ls.c -o ./build/main.cgi -lpthread
gcc -g main.c cJSON.c add.c ls.c usage.c  -o ./build/main.cgi -lpthread
```

個別編譯

```console
gcc -c main.c -o main.o
gcc -c cJSON.c -o cJSON.o
gcc -c add.c -o add.o
gcc main.o cJSON.o add.o -o ./build/main.cgi
```

用 make 編譯

```console
make
```

### 環境

- gcc (Debian 4.9.2-10) 4.9.2
