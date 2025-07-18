import tkinter as tk
from tkinter import ttk, messagebox
from PIL import Image, ImageTk, ImageDraw, ImageEnhance, ImageFilter
import os
import time
from datetime import datetime
import platform
import sys

class LinuxMintLogin:
    def __init__(self, root, background_image_path=None):
        self.root = root
        self.background_image_path = background_image_path
        self.password_var = tk.StringVar()
        self.setup_window()
        self.create_widgets()
        
    def setup_window(self):
        self.root.title("Linux Mint Login")
        self.root.attributes('-fullscreen', True)
        self.screen_width = self.root.winfo_screenwidth()
        self.screen_height = self.root.winfo_screenheight()
        self.root.bind('<Escape>', self.exit_fullscreen)
        
    def load_background_image(self):
        """Load and process background image"""
        if self.background_image_path and os.path.exists(self.background_image_path):
            try:
                image = Image.open(self.background_image_path)
                image = image.resize((self.screen_width, self.screen_height), Image.Resampling.LANCZOS)
                
                # Apply visual effects
                blurred_image = image.filter(ImageFilter.GaussianBlur(radius=2))
                enhancer = ImageEnhance.Brightness(blurred_image)
                darkened_image = enhancer.enhance(0.8)
                
                return ImageTk.PhotoImage(darkened_image)
            except Exception as e:
                print(f"Error loading background image: {e}")
                return None
        return None
        
    def create_widgets(self):
        # Main container with solid background color
        self.main_frame = tk.Frame(self.root, bg='#2d3748')
        self.main_frame.pack(expand=True, fill='both')
        
        # Background image or solid color
        self.bg_image = self.load_background_image()
        if self.bg_image:
            self.bg_label = tk.Label(self.main_frame, image=self.bg_image)
            self.bg_label.image = self.bg_image
            self.bg_label.place(x=0, y=0, relwidth=1, relheight=1)
        
        # Top panel with solid dark color
        self.create_top_panel()
        
        # Center login area
        self.create_login_area()
        
        # Bottom panel
        self.create_bottom_panel()
        
        # Focus on password entry
        self.password_entry.focus_set()
    
    def create_top_panel(self):
        # Using solid color instead of rgba
        top_panel = tk.Frame(self.main_frame, bg='#2d3748', height=35)
        top_panel.pack(fill='x', side='top')
        top_panel.pack_propagate(False)
        
        # Hostname
        hostname = platform.node() if hasattr(platform, 'node') else 'mint21'
        hostname_label = tk.Label(top_panel, text=hostname, 
                                font=('Ubuntu', 11), 
                                fg='white', bg='#2d3748')
        hostname_label.pack(side='left', padx=15, pady=8)
        
        # Right side items
        top_right_frame = tk.Frame(top_panel, bg='#2d3748')
        top_right_frame.pack(side='right', padx=15, pady=8)
        
        # Language selector
        lang_label = tk.Label(top_right_frame, text="us", 
                            font=('Ubuntu', 10), 
                            fg='white', bg='#2d3748')
        lang_label.pack(side='right', padx=5)
        
        # Time
        self.time_label = tk.Label(top_right_frame, text="", 
                                font=('Ubuntu', 11), 
                                fg='white', bg='#2d3748')
        self.time_label.pack(side='right', padx=10)
        self.update_clock()
        
        # Power button
        power_btn = tk.Label(top_right_frame, text="⏻", 
                           font=('Ubuntu', 12), 
                           fg='white', bg='#2d3748',
                           cursor='hand2')
        power_btn.pack(side='right', padx=5)
        power_btn.bind('<Button-1>', self.show_power_menu)
    
    def create_login_area(self):
        # Using solid light blue background for login area
        login_container = tk.Frame(self.main_frame, bg='#87ceeb')
        login_container.place(relx=0.5, rely=0.5, anchor='center')
        
        # User avatar
        avatar_frame = tk.Frame(login_container, bg='#87ceeb')
        avatar_frame.pack(pady=(0, 20))
        
        # Create circular avatar
        avatar_img = Image.new('RGBA', (100, 100), (0, 0, 0, 0))
        draw = ImageDraw.Draw(avatar_img)
        draw.ellipse((10, 10, 90, 90), fill=(74, 144, 226))
        
        avatar_tk = ImageTk.PhotoImage(avatar_img)
        avatar_label = tk.Label(avatar_frame, image=avatar_tk, bg='#87ceeb')
        avatar_label.image = avatar_tk
        avatar_label.pack()
        
        # Username
        username_label = tk.Label(login_container, text="merilyn", 
                                font=('Ubuntu', 18), 
                                fg='white', bg='#87ceeb')
        username_label.pack(pady=(0, 20))
        
        # Password entry
        self.create_password_entry(login_container)
        
        # Error message
        self.error_label = tk.Label(login_container, text="", 
                                  font=('Ubuntu', 12),
                                  fg='#ff4444', bg='#87ceeb')
        self.error_label.pack(pady=(10, 0))
        
        # Guest session
        guest_btn = tk.Label(login_container, text="Guest Session", 
                           font=('Ubuntu', 12, 'underline'),
                           fg='white', bg='#87ceeb',
                           cursor='hand2')
        guest_btn.pack(pady=(30, 0))
        guest_btn.bind('<Button-1>', self.guest_login)
    
    def create_password_entry(self, parent):
        entry_frame = tk.Frame(parent, bg='white', bd=0)
        entry_frame.pack()
        
        inner_frame = tk.Frame(entry_frame, bg='white')
        inner_frame.pack(padx=1, pady=1)
        
        self.password_entry = tk.Entry(inner_frame, 
                                    textvariable=self.password_var,
                                    font=('Ubuntu', 14),
                                    show='•',
                                    bg='white',
                                    fg='#333333',
                                    bd=0,
                                    width=25)
        self.password_entry.pack(side='left', padx=10, pady=8)
        self.password_entry.bind('<Return>', self.handle_login)
        
        login_btn = tk.Label(inner_frame, text="→", 
                           font=('Ubuntu', 16, 'bold'),
                           fg='#4a90e2', bg='white',
                           cursor='hand2')
        login_btn.pack(side='right', padx=10)
        login_btn.bind('<Button-1>', self.handle_login)
    
    def create_bottom_panel(self):
        bottom_panel = tk.Frame(self.main_frame, bg='#2d3748')
        bottom_panel.pack(fill='x', side='bottom', pady=20)
        
        access_btn = tk.Label(bottom_panel, text="♿", 
                            font=('Ubuntu', 14),
                            fg='white', bg='#2d3748',
                            cursor='hand2')
        access_btn.pack(side='left', padx=20)
        
        session_btn = tk.Label(bottom_panel, text="Cinnamon", 
                             font=('Ubuntu', 11),
                             fg='white', bg='#2d3748',
                             cursor='hand2')
        session_btn.pack(side='right', padx=20)
    
    def update_clock(self):
        current_time = datetime.now().strftime("%H:%M")
        self.time_label.config(text=current_time)
        self.root.after(1000, self.update_clock)
    
    def show_power_menu(self, event):
        menu = tk.Menu(self.root, tearoff=0, bg='#2d3748', fg='white',
                      activebackground='#4a90e2', activeforeground='white')
        menu.add_command(label="Restart", command=self.restart_system)
        menu.add_command(label="Shut Down", command=self.shutdown_system)
        menu.add_command(label="Suspend", command=self.suspend_system)
        menu.post(event.x_root, event.y_root)
    
    def handle_login(self, event=None):
        password = self.password_var.get().strip()
        
        if not password:
            self.error_label.config(text="Please enter your password")
            return
            
        self.error_label.config(text="Authenticating...", fg='white')
        self.password_entry.config(state='disabled')
        self.root.update()
        
        self.root.after(1500, lambda: self.authenticate(password))
    
    def authenticate(self, password):
        if password == "mint":
            self.login_success()
        else:
            self.login_failed()
    
    def login_success(self):
        self.error_label.config(text="Login successful!", fg='#44ff44')
        self.root.after(1000, self.exit_fullscreen)
    
    def login_failed(self):
        self.error_label.config(text="Invalid password", fg='#ff4444')
        self.password_entry.config(state='normal')
        self.password_var.set("")
        self.password_entry.focus_set()
        self.shake_login()
    
    def shake_login(self):
        x, y = self.main_frame.winfo_rootx(), self.main_frame.winfo_rooty()
        for _ in range(2):
            for offset in [10, -10, 5, -5, 0]:
                self.root.geometry(f"+{x + offset}+{y}")
                self.root.update()
                time.sleep(0.03)
    
    def guest_login(self, event=None):
        self.error_label.config(text="Starting guest session...", fg='white')
        self.root.after(1500, self.exit_fullscreen)
    
    def restart_system(self):
        if messagebox.askyesno("Restart", "Are you sure you want to restart the system?"):
            self.root.destroy()
    
    def shutdown_system(self):
        if messagebox.askyesno("Shut Down", "Are you sure you want to shut down the system?"):
            self.root.destroy()
    
    def suspend_system(self):
        messagebox.showinfo("Suspend", "System will suspend")
        self.root.destroy()
    
    def exit_fullscreen(self, event=None):
        self.root.attributes('-fullscreen', False)
        self.root.geometry('800x600')
        messagebox.showinfo("Login", "Welcome to Linux Mint!")
        self.root.quit()

if __name__ == "__main__":
    root = tk.Tk()
    
    # Set your background image path here
    bg_path = "/home/aelomari/Downloads/1_gVgI8uSoXdOgT3ptV16Vlw.jpg"  # Replace with your image path
    
    login = LinuxMintLogin(root, background_image_path=bg_path)
    login.root.mainloop()