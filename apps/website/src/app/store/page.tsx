import { Footer } from "@/components/footer";
import { Navbar } from "@/components/navbar";
import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Store",
  description:
    "Browse skins, themes and cosmetics for your Pk_Launcher setup.",
};

const PRODUCTS = [
  {
    name: "Crimson Skin Pack",
    category: "Skin",
    price: 120,
    size: "450 MB",
    description: "Five premium HD skins with a red-and-black theme.",
  },
  {
    name: "Midnight Theme",
    category: "Theme",
    price: 80,
    size: "12 MB",
    description: "A dark, sleek UI theme for the launcher.",
  },
  {
    name: "Neon Capes Pack",
    category: "Cosmetic",
    price: 200,
    size: "6 MB",
    description: "Animated capes with neon glow effects.",
  },
  {
    name: "Vanilla+ Texture Pack",
    category: "Resource Pack",
    price: 0,
    size: "48 MB",
    description: "Free high-resolution textures that keep the vanilla feel.",
  },
] as const;

function formatPrice(price: number): string {
  return price === 0 ? "Free" : `${price} coins`;
}

export default function StorePage() {
  return (
    <>
      <Navbar />
      <main className="mx-auto max-w-6xl px-6 py-20">
        <div className="mx-auto max-w-2xl text-center">
          <p className="text-sm font-medium tracking-widest text-text-accent uppercase">
            Customise your launcher
          </p>
          <h1 className="mt-4 text-4xl font-extrabold tracking-tight text-white md:text-5xl">
            The <span className="text-brand">Store</span>
          </h1>
          <p className="mt-4 text-lg text-text-secondary">
            Skins, themes, capes and resource packs — all synced across your
            launcher, website and Discord account.
          </p>
        </div>

        <div className="mt-16 grid gap-6 sm:grid-cols-2 lg:grid-cols-4">
          {PRODUCTS.map((product) => (
            <article
              key={product.name}
              className="flex flex-col rounded-xl border border-border bg-surface-2 p-6 transition-all duration-200 hover:border-brand hover:shadow-glow-sm"
            >
              <div className="flex items-center justify-between">
                <span className="rounded bg-surface-4 px-2 py-0.5 text-xs font-medium text-text-secondary">
                  {product.category}
                </span>
              </div>
              <h2 className="mt-4 text-lg font-bold text-white">{product.name}</h2>
              <p className="mt-2 flex-1 text-sm leading-relaxed text-text-secondary">
                {product.description}
              </p>
              <p className="mt-4 text-xs text-text-muted">{product.size}</p>
              <span className="mt-4 inline-flex items-center justify-center rounded bg-brand px-4 py-2.5 text-sm font-semibold text-white transition-colors duration-200 hover:bg-brand-red-hover">
                {formatPrice(product.price)} — {product.size}
              </span>
            </article>
          ))}
        </div>
      </main>
      <Footer />
    </>
  );
}