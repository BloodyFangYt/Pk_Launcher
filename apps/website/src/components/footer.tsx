import Link from "next/link";

const FOOTER_LINKS = [
  { href: "/download", label: "Download" },
  { href: "/store", label: "Store" },
  { href: "/login", label: "Login" },
] as const;

export function Footer() {
  return (
    <footer className="border-t border-border bg-surface-1">
      <div className="mx-auto flex max-w-6xl flex-col items-center justify-between gap-6 px-6 py-10 md:flex-row">
        <div>
          <p className="text-sm font-bold">
            <span className="text-white">Pk</span>
            <span className="text-brand">Launcher</span>
          </p>
          <p className="mt-1 text-xs text-text-muted">
            A fast, modern Minecraft launcher.
          </p>
        </div>

        <ul className="flex items-center gap-6">
          {FOOTER_LINKS.map((link) => (
            <li key={link.href}>
              <Link
                href={link.href}
                className="text-sm text-text-secondary transition-colors duration-200 hover:text-white"
              >
                {link.label}
              </Link>
            </li>
          ))}
        </ul>

        <p className="text-xs text-text-muted">
          &copy; {new Date().getFullYear()} Pk_Launcher. All rights reserved.
        </p>
      </div>
    </footer>
  );
}