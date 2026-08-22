import "dotenv/config";
import {
  Client,
  Collection,
  Events,
  GatewayIntentBits,
  REST,
  Routes,
} from "discord.js";
import { BotApiClient } from "./backend/client.js";
import { commands } from "./commands/index.js";
import type { Command } from "./types.js";

declare module "discord.js" {
  interface Client {
    commands: Collection<string, Command>;
  }
}

const TOKEN = process.env.DISCORD_TOKEN;
const APPLICATION_ID = process.env.DISCORD_APPLICATION_ID;
const GUILD_ID = process.env.DISCORD_GUILD_ID;

if (!TOKEN) {
  console.error("Missing DISCORD_TOKEN. See .env.example");
  process.exit(1);
}

const client = new Client({
  intents: [GatewayIntentBits.Guilds],
});
client.commands = new Collection(
  commands.map((command) => [command.data.name, command]),
);

const api = new BotApiClient();

/** Register application (/) commands with Discord via the REST API. */
async function registerCommands(): Promise<void> {
  const id = APPLICATION_ID ?? client.application?.id;
  if (!id) {
    console.warn(
      "DISCORD_APPLICATION_ID not set - skipping slash-command registration.",
    );
    return;
  }

  const rest = new REST({ version: "10" }).setToken(TOKEN!);
  const body = commands.map((command) => command.data.toJSON());
  const route = GUILD_ID
    ? Routes.applicationGuildCommands(id, GUILD_ID)
    : Routes.applicationCommands(id);

  await rest.put(route, { body });
  console.log(
    `Registered ${body.length} slash command(s) - ${
      GUILD_ID ? `guild ${GUILD_ID}` : "globally"
    }.`,
  );
}

client.once(Events.ClientReady, async (c) => {
  console.log(`Logged in as ${c.user.tag} (id ${c.user.id})`);
  try {
    await registerCommands();
  } catch (error) {
    console.error("Failed to register slash commands:", error);
  }
});

client.on(Events.InteractionCreate, async (interaction) => {
  if (!interaction.isChatInputCommand()) return;

  const command = client.commands.get(interaction.commandName);
  if (!command) {
    await interaction
      .reply({ content: "Unknown command.", ephemeral: true })
      .catch(() => undefined);
    return;
  }

  try {
    await command.execute(
      interaction as Parameters<typeof command.execute>[0],
      api as Parameters<typeof command.execute>[1],
    );
  } catch (error) {
    console.error(`Error while running /${interaction.commandName}:`, error);
    const content = "Something went wrong while running this command.";
    if (interaction.replied || interaction.deferred) {
      await interaction.editReply({ content }).catch(() => undefined);
    } else {
      await interaction
        .reply({ content, ephemeral: true })
        .catch(() => undefined);
    }
  }
});

void client.login(TOKEN);