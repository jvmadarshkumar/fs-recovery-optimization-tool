from flask import Flask, request, jsonify, render_template
import subprocess
import os

app = Flask(__name__, template_folder="templates", static_folder="static")

FS_BINARY = os.path.join("..", "fs_tool.exe")

def run_fs_commands(commands, timeout=8):
    if not os.path.exists(FS_BINARY):
        return {"ok": False, "out": "fs_tool.exe not found. Compile it first."}

    input_str = "\n".join(commands + ["exit"]) + "\n"

    try:
        proc = subprocess.Popen(
            [FS_BINARY],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        out, _ = proc.communicate(input=input_str, timeout=timeout)
        return {"ok": True, "out": out}
    except:
        return {"ok": False, "out": "Execution error or timeout."}

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/api/exec", methods=["POST"])
def api_exec():
    data = request.get_json() or {}
    cmds = data.get("commands")
    if not isinstance(cmds, list):
        return jsonify({"ok": False, "out": "Commands must be a list."})
    return jsonify(run_fs_commands(cmds))

if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000, debug=False)
