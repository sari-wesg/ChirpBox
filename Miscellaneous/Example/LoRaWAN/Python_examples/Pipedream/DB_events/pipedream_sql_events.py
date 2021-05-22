# TODO:
# 1. Replace PIPEDREAM_API_KEY
# 2. Change "lorawan_chirpbox" to your table name
# 3. Change ASC/DESC to sort the result in ascending/descending order
# 4. Change LIMIT number

import requests

headers = {
    'Authorization': 'Bearer PIPEDREAM_API_KEY',
    'Content-Type': 'application/json',
}

data = '{"query": "SELECT * FROM lorawan_chirpbox ORDER BY context.ts DESC LIMIT 3"}'

response = requests.post('https://rt.pipedream.com/sql', headers=headers, data=data)

print(response.text)
