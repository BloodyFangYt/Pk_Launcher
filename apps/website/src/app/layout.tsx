import type { Metadata } from "next";

import "./globals.css";

export const metadata: Metadata = {
  title: {
    default: "Pk_Launcher — Minecraft Launcher",
    template: "%s — Pk_Launcher",
  },
  description:
    "Pk_Launcher — the fast, modern Minecraft launcher. Download, manage versions, mods and accounts in one place.",
  keywords: ["minecraft", "launcher", "mods", "forge", "fabric", "pk launcher"],
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body className="bg-surface-0 text-text-primary antialiased">
        {children}
      </body>
    </html>
  );
}