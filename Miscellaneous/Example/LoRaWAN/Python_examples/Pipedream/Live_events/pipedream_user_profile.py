# TODO:
# 1. Replace PIPEDREAM_API_KEY

import requests

headers = {
    'Authorization': 'Bearer PIPEDREAM_API_KEY',
}

response = requests.get('https://api.pipedream.com/v1/users/me', headers=headers)


print(response.text)
