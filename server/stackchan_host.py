


# ｽﾀｯｸﾁｬﾝからのリクエストに応答するためのHTTPサーバ
from fastapi import FastAPI, Request
from fastapi.responses import JSONResponse,HTMLResponse,FileResponse
from starlette.middleware.cors import CORSMiddleware 

from pydantic import BaseModel
from typing import List, Union

import asyncio
import uvicorn
import json
import uuid
import time
import os
import socket

app = FastAPI()
port_to_listen = 8080

public_http_path = "./public_http" 


# CORS対策
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,   # 追記により追加
    allow_methods=["*"],      # 追記により追加
    allow_headers=["*"]       # 追記により追加
)



def get_file_from_path(path):

    path = f"{public_http_path}/{path}"

    return FileResponse(path)


def send_udp_file_stream(target_host, target_port, file_path):

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


    sender_port = 4423

    bytes_of_paket = 1024

    with open(file_path, 'rb') as f:
        while True:
            chunk = f.read(bytes_of_paket)  # 1024バイトずつ読み込む
            if not chunk:
                break  # ファイルの終わりに達したらループを抜ける
            
            print(f"Sending chunk of size {len(chunk)} bytes to {target_host}:{target_port}")
            sock.sendto(chunk, (target_host, target_port))
            
            # 送った後、再生が終わったらsender_portにNEXTが来るのを待つ
            sock.bind(('', sender_port))
            print(f"Waiting for NEXT on port {sender_port}...")
            while True:
                data, addr = sock.recvfrom(1024)
                if data.decode() == "NEXT":
                    print("Received NEXT, sending next chunk...")
                    break
                else:
                    print(f"Received unexpected message: {data.decode()} from {addr}")

            
            time.sleep(0.01)  # 送信間隔を調整（必要に応じて変更）

    # ファイルの送信が完了したら[EOS]を送信
    sock.sendto(b'[EOS]', (target_host, target_port))


    # ソケットを閉じる

    sock.close()

    return







# HTTPで静的ファイルを読み、返す
def get_file( request:Request):

    path = request.url.path[1:]  # 先頭のスラッシュを削除してパスを取得
    if path == "":
        path = "index.html"  # デフォルトのファイル名を指定

    path = f"{public_http_path}/{path}"

    allowed_extensions = [".html", ".css", ".js"]
    if not any(path.endswith(ext) for ext in allowed_extensions):
        return HTMLResponse(content="Filetype not found", status_code=404)


    print("get file:{}".format(path))

    try:
        return get_file_from_path(path) #FileResponse(path)
        #return HTMLResponse(content=content, status_code=200)
    except FileNotFoundError:
        return HTMLResponse(content="File not found", status_code=404)
    



@app.get("/")
async def get_root(request: Request):
    return get_file(request)


# {file_path:path} がワイルドカードとして機能
@app.get("/files/{file_path:path}")
async def read_file(file_path: str):

    return get_file_from_path(file_path)


@app.post("/stream_audio/{file_path:path}")
async def get_audio_stream(request: Request, file_path:str):

    file_path = f"{public_http_path}/{file_path}"
    

    return_port = int(request.query_params.get("recv_port", 4423))


    if not os.path.isfile(file_path):
        return JSONResponse(content={"error": "File not found"}, status_code=404)


    send_udp_file_stream( file_path=file_path, target_host=request.client.host, target_port=return_port )


    return JSONResponse(content={"message": "Audio stream sent"}, status_code=200)





if __name__ == "__main__":


    # このスクリプトの存在するパスを取得
    import os
    here_path = os.path.abspath(__file__)
    print(f"here_path:{here_path}")



    # root_pathはこのスクリプトが動作しているパスに合わせて変更する
    uvicorn.run(app, host="0.0.0.0", port=port_to_listen, log_level="info")
