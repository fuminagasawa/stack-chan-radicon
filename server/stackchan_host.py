


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

  


if __name__ == "__main__":


    # このスクリプトの存在するパスを取得
    import os
    here_path = os.path.abspath(__file__)
    print(f"here_path:{here_path}")



    # root_pathはこのスクリプトが動作しているパスに合わせて変更する
    uvicorn.run(app, host="0.0.0.0", port=port_to_listen, log_level="info")
