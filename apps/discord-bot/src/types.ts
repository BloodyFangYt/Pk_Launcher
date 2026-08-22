import type {
  ChatInputCommandInteraction,
  SlashCommandBuilder,
} from "discord.js";
import type { BotApiClient } from "./backend/client.js";

/**
 * A single slash command. Kept dependency-light so each command can be
 * self-documented, registered, and executed against the bot backend client.
 */
export interface Command {
  data: Pick<
    SlashCommandBuilder,
    "name" | "description" | "toJSON"
  >;
  execute: (
    interaction: ChatInputCommandInteraction,
    api: BotApiClient,
  ) => Promise<void>;
}