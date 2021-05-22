# TODO:
# 1. Replace PIPEDREAM_API_KEY
# 2. change "dc_OLuJQdB" to your source id

import requests

headers = {
    'Authorization': 'Bearer PIPEDREAM_API_KEY',
}

params = (
    ('expand', 'event'),
)

response = requests.get('https://api.pipedream.com/v1/sources/dc_OLuJQdB/event_summaries', headers=headers, params=params)

print(response.text)
