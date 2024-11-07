curl -X POST -d '{"vector": [0.8], "id":10, "index_type":1, "fields": {"aaa": 19}}' http://localhost:7123/VdbService/http/upsert
curl -X POST -d '{"vector": [0.8], "id":10, "index_type":1, "fields": {"aaa": 20}}' http://localhost:7123/VdbService/http/upsert
curl -X POST -d '{"vector": [0.5], "k":2, "index_type":1, "condition": {"field":"aaa", "op":"=", "value": 19 }}' http://localhost:7123/VdbService/http/search
curl -X POST -d '{"vector": [0.5], "k":2, "index_type":1, "condition": {"field":"aaa", "op":"!=", "value": 19 }}' http://localhost:7123/VdbService/http/search
curl -X POST -d '{"id":10}' http://localhost:7123/VdbService/http/query
curl -X POST -d '{"vector": [0.3], "id":11, "index_type":2, "fields": {"bbb": 11}}' http://localhost:7123/VdbService/http/upsert
curl -X POST -d '{"id":11}' http://localhost:7123/VdbService/http/query
curl -X POST -d '{"vector": [0.5], "k":2, "index_type":2, "condition": {"field":"bbb", "op":"=", "value": 11 }}' http://localhost:7123/VdbService/http/search
curl -X POST -d '{}' http://localhost:7123/VdbService/http/snapshot
