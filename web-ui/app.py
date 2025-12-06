from flask import Flask, request, jsonify, render_template
import subprocess
import threading
import queue
import time
import os

app = Flask(__name__, template_folder="templates", static_folder="static")

FS_BINARY = os.path.join("..", "fs_tool.exe")

# GLOBAL persistent process
process = None
process_lock = threading.Lock()
output_queue = queue.Queue()

def start_process():
    global process
    if process is None or process.poll() is not None:
        process = subprocess.Popen(
            [FS_BINARY],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )
        threading.Thread(target=read_output, daemon=True).start()

def read_output():
    global process
    while True:
        line = process.stdout.readline()
        if not line:
            break
        output_queue.put(line)

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/api/exec", methods=["POST"])
def api_exec():
    global process
    cmds = request.json.get("commands", [])

    with process_lock:
        start_process()
        for cmd in cmds:
            process.stdin.write(cmd + "\n")
            process.stdin.flush()

        time.sleep(0.2)

        out = ""
        while not output_queue.empty():
            out += output_queue.get()

    return jsonify({"ok": True, "out": out})

if __name__ == "__main__":
    start_process()
    app.run(host="127.0.0.1", port=5000, debug=False)
