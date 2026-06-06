#!/usr/bin/env python3
"""Generate simple test assets for the visual novel player."""

import os
import struct
import wave
import math

# Create directories
os.makedirs("src/test/assets/img", exist_ok=True)
os.makedirs("src/test/assets/audio", exist_ok=True)

# Generate a simple 800x600 PNG with a gradient background
try:
    from PIL import Image, ImageDraw
    
    # Background - blue gradient
    img = Image.new('RGB', (800, 600), color='darkblue')
    draw = ImageDraw.Draw(img)
    for y in range(600):
        r = int(20 + (y / 600) * 40)
        g = int(40 + (y / 600) * 60)
        b = int(100 + (y / 600) * 80)
        draw.line([(0, y), (800, y)], fill=(r, g, b))
    
    # Add some shapes to make it interesting
    draw.ellipse([100, 100, 300, 300], fill=(255, 200, 100), outline=(255, 150, 50), width=3)
    draw.rectangle([500, 200, 700, 400], fill=(100, 255, 150), outline=(50, 200, 100), width=3)
    
    img.save("src/test/assets/img/background.png")
    print("Generated: src/test/assets/img/background.png")
    
    # Character sprite - simple circle portrait
    sprite = Image.new('RGBA', (256, 256), color=(0, 0, 0, 0))
    draw_s = ImageDraw.Draw(sprite)
    draw_s.ellipse([10, 10, 246, 246], fill=(255, 180, 160), outline=(200, 120, 100), width=4)
    draw_s.ellipse([80, 70, 110, 100], fill=(50, 50, 80))  # left eye
    draw_s.ellipse([146, 70, 176, 100], fill=(50, 50, 80))  # right eye
    draw_s.arc([80, 120, 176, 180], start=0, end=180, fill=(180, 80, 80), width=4)  # smile
    
    sprite.save("src/test/assets/img/character.png")
    print("Generated: src/test/assets/img/character.png")
    
except ImportError:
    print("Pillow not available, skipping image generation")
    print("Install with: pip3 install Pillow")

# Generate a simple WAV file (sine wave beep)
def generate_wav(filename, duration=2.0, freq=440.0, sample_rate=44100):
    num_samples = int(duration * sample_rate)
    amplitude = 32767 * 0.3  # 30% volume
    
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)  # Mono
        wav_file.setsampwidth(2)    # 16-bit
        wav_file.setframerate(sample_rate)
        
        for i in range(num_samples):
            # Sine wave with decay
            t = i / sample_rate
            decay = max(0, 1.0 - t / duration)
            value = int(amplitude * decay * math.sin(2 * math.pi * freq * t))
            wav_file.writeframes(struct.pack('<h', value))

wav_file = "src/test/assets/audio/effect.wav"
generate_wav(wav_file, duration=0.5, freq=880)
print(f"Generated: {wav_file}")

# Create a longer ambient WAV for music
music_file = "src/test/assets/audio/music.wav"
generate_wav(music_file, duration=3.0, freq=220)
print(f"Generated: {music_file}")

# Note: Raylib supports WAV natively, so we use WAV instead of OGG
# The test stories reference .ogg but we can update them or create new ones

print("\nDone! Test assets created in src/test/assets/")
print("\nTo use these assets, compile a story that references them:")
print('  asset bg_test = "src/test/assets/img/background.png";')
print('  show background bg_test;')
