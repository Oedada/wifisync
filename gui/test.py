import requests

response = requests.post(
            "http://127.0.0.1:5000/connect",
            json={
                "uuid": "fkdsjf"
            }
        )
print(response.status_code)
