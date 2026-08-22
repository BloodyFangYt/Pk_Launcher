import type { Config } from "tailwindcss";

const config: Config = {
  content: ["./src/**/*.{js,ts,jsx,tsx,mdx}"],
  theme: {
    extend: {
      colors: {
        brand: {
          red: "#DC2626",
          "red-hover": "#B91C1C",
          "red-light": "#FCA5A5",
          "red-glow": "#DC262640",
          black: "#000000",
          white: "#FFFFFF",
        },
        surface: {
          0: "#000000",
          1: "#0A0A0A",
          2: "#111111",
          3: "#1A1A1A",
          4: "#222222",
          5: "#2A2A2A",
        },
        text: {
          primary: "#FFFFFF",
          secondary: "#A0A0A0",
          muted: "#666666",
          accent: "#DC2626",
        },
        border: {
          DEFAULT: "#2A2A2A",
          hover: "#DC2626",
          focus: "#DC2626",
        },
      },
      fontFamily: {
        sans: ["Inter", "system-ui", "-apple-system", "sans-serif"],
        mono: ["JetBrains Mono", "Fira Code", "Consolas", "monospace"],
      },
      boxShadow: {
        "glow-sm": "0 0 20px #DC262640, 0 0 60px #DC262620",
        "glow-lg": "0 0 40px #DC262660, 0 0 80px #DC262630",
      },
      keyframes: {
        "glow-pulse": {
          "0%, 100%": { boxShadow: "0 0 20px #DC262640" },
          "50%": { boxShadow: "0 0 40px #DC262660, 0 0 80px #DC262630" },
        },
      },
      animation: {
        "glow-pulse": "glow-pulse 2s ease-in-out infinite",
      },
    },
  },
  plugins: [],
};

export default config;