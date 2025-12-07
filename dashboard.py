import tkinter as tk
import os

BLOCK_SIZE = 25   
REFRESH_RATE = 200 

class DiskVisualizer:
    def __init__(self, root):
        self.root = root
        self.root.title("File System Visualizer")
        self.root.geometry("900x600")
        self.root.configure(bg="#2c3e50")
        
        tk.Label(root, text="Live Disk Map", font=("Segoe UI", 18, "bold"), fg="white", bg="#2c3e50").pack(pady=10)
        
        self.legend = tk.Frame(root, bg="#2c3e50")
        self.legend.pack(pady=5)
        self.add_legend("🟨 System", "#f1c40f")
        self.add_legend("🟩 Free", "#2ecc71")
        self.add_legend("🟥 Used", "#e74c3c")

        self.canvas = tk.Canvas(root, width=850, height=450, bg="#34495e", highlightthickness=0)
        self.canvas.pack(pady=20)
        
        self.status = tk.Label(root, text="Waiting...", font=("Consolas", 10), fg="#95a5a6", bg="#2c3e50")
        self.status.pack(pady=5)
        self.update_grid()

    def add_legend(self, text, color):
        tk.Label(self.legend, text=text, fg=color, bg="#2c3e50", font=("Arial", 10, "bold")).pack(side=tk.LEFT, padx=15)

    def update_grid(self):
        try:
            if os.path.exists("disk_map.txt"):
                with open("disk_map.txt", "r") as f:
                    self.draw(f.read().strip())
                    self.status.config(text="Disk Loaded")
            else: self.status.config(text="Waiting for C++...")
        except: pass
        self.root.after(REFRESH_RATE, self.update_grid)

    def draw(self, bitmap):
        self.canvas.delete("all") 
        x, y = 10, 10
        for i, bit in enumerate(bitmap):
            color = "#f1c40f" if i < 4 else ("#e74c3c" if bit == '1' else "#2ecc71")
            self.canvas.create_rectangle(x, y, x+BLOCK_SIZE, y+BLOCK_SIZE, fill=color, outline="#2c3e50")
            x += BLOCK_SIZE + 2
            if x > 800: x = 10; y += BLOCK_SIZE + 2

if __name__ == "__main__":
    root = tk.Tk()
    DiskVisualizer(root)
    root.mainloop()