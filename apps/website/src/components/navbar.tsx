import Link from "next/link";

const NAV_LINKS = [
  { href: "/", label: "Home" },
  { href: "/download", label: "Download" },
  { href: "/store", label: "Store" },
  { href: "/login", label: "Login" },
] as const;

export function Navbar() {
  return (
    <header className="sticky top-0 z-50 border-b border-border bg-surface-1/90 backdrop-blur">
      <nav
        className="mx-auto flex h-16 max-w-6xl items-center justify-between px-6"
        aria-label="Primary"
      >
        <Link href="/" className="flex items-center gap-2">
          <span className="flex h-8 w-8 items-center justify-center rounded bg-brand-red font-mono text-lg font-bold text-white">
            P
          </span>
          <span className="text-lg font-black tracking-tight">
            <span className="text-white">Pk</span>
            <span className="text-brand">Launcher</span>
          </span>
        </Link>

        <ul className="hidden items-center gap-8 md:flex">
          {NAV_LINKS.map((link) => (
            <li key={link.href}>
              <Link
                href={link.href}
                className="text-sm font-medium text-text-secondary transition-colors duration-200 hover:text-white"
              >
                {link.label}
              </Link>
            </li>
          ))}
        </ul>

        <Link
          href="/download"
          className="rounded bg-brand px-4 py-2 text-sm font-semibold text-white transition-colors duration-200 hover:bg-brand-red-hover"
        >
          Get Launcher
        </Link>
      </nav>
    </header>
  );
}