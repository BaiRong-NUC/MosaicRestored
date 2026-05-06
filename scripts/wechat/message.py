import json
import requests
import os
from dotenv import load_dotenv


# 向用户发送消息
class Message:
    def __init__(self, access_token, user_id):
        self.access_token = access_token
        self.user_id = user_id
        self.url = f"https://api.weixin.qq.com/cgi-bin/message/custom/send?access_token={self.access_token}"
        self.default_text = "Hello, this is a test message!"

    # 输入消息内容，返回(errcode, errmsg)
    def send_message(self, content):
        text = content if content else self.default_text
        payload = {
            "touser": self.user_id,
            "msgtype": "text",
            "text": {"content": text},
        }

        response = requests.post(
            self.url,
            data=json.dumps(payload, ensure_ascii=False).encode("utf-8"),
            headers={"Content-Type": "application/json; charset=utf-8"},
        )
        result = response.json()
        return result.get("errcode", -1), result.get("errmsg", "")
