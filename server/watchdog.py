import json
import time
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

def read_diffs(path, last_pos):
    diffs = []
    with open(path, "r", encoding="utf-8") as f:
        f.seek(last_pos)
        for line in f:
            diffs.append(json.loads(line))
        new_pos = f.tell()
    return diffs, new_pos


class JsonlHandler(FileSystemEventHandler):
    def __init__(self, path):
        self.path = path
        self.last_pos = 0
        self.state = {}

    def on_modified(self, event):
        if event.src_path.endswith(self.path):
            diffs, self.last_pos = read_diffs(self.path, self.last_pos)
            for d in diffs:
                self.state.update(d)
            print("updated state:", self.state)


handler = JsonlHandler("config.jsonl")
observer = Observer()
observer.schedule(handler, path=".", recursive=False)
observer.start()

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    observer.stop()

observer.join()