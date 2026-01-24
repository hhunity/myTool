import { defineConfig } from "vite";

export default defineConfig({
  server: {
    proxy: {
      "/api": "http://127.0.0.1:8080",
      "/stream.mjpg": "http://127.0.0.1:8080",
      // もし /frame.jpg なども使うなら同様に追加
      // "/frame.jpg": "http://127.0.0.1:8080",
    },
  },
});