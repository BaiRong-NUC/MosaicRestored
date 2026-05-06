import requests
import os
from dotenv import load_dotenv
from wechat.message import Message


class TokenInfo:
    # refresh参数用于决定是否强制刷新token，默认为False
    def __init__(self, refresh=False):
        load_dotenv()
        self.app_id = os.getenv("APP_ID")
        self.app_secret = os.getenv("APP_SECRET")
        self.user_id = os.getenv("USER_ID")
        self.access_token = None

        if refresh:
            self.refresh_access_token()
        else:
            self.access_token = os.getenv("ACCESS_TOKEN")

    def refresh_access_token(self):
        access_token_url = f"https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential&appid={self.app_id}&secret={self.app_secret}"
        result = requests.get(access_token_url, timeout=10).json()
        access_token = result.get("access_token")

        if not access_token:
            errcode = result.get("errcode", -1)
            errmsg = result.get("errmsg", "")
            raise RuntimeError(
                f"Failed to refresh access_token, error code: {errcode}, error message: {errmsg}"
            )

        self.access_token = access_token
        return self.access_token


class User:
    def __init__(self, refresh_token=False):
        self.token_info = TokenInfo(refresh=refresh_token)
        self.reset_message()

    def reset_message(self):
        self.message = Message(self.token_info.access_token, self.token_info.user_id)

    def refresh_access_token(self):
        self.token_info.refresh_access_token()
        self.reset_message()

    def send_message(self, content):
        errcode, error_message = self.message.send_message(content)
        if errcode == 0:
            print("Message sent successfully!")
            return True

        print(
            f"Failed to send message, error code: {errcode}, error message: {error_message}. Refreshing access_token and retrying..."
        )
        if errcode in (
            40001,
            42001,
        ):  # 40001: invalid credential, 42001: access_token expired
            try:
                self.refresh_access_token()
            except RuntimeError as error:
                print(error)
                return False

            errcode, error_message = self.message.send_message(content)
            if errcode == 0:
                print("Message sent successfully after refreshing access_token!")
                return True

            print(
                f"Failed to send message after refreshing access_token, error code: {errcode}, error message: {error_message}"
            )
            return False

        return False
