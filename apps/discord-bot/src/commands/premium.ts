import { EmbedBuilder, SlashCommandBuilder } from "discord.js";
import { ApiError } from "../backend/client.js";
import type { Command } from "../types.js";

/** /premium — check whether your PK Launcher account has Premium. */
export const premium: Command = {
  data: new SlashCommandBuilder()
    .setName("premium")
    .setDescription("Check whether your account has PK Launcher Premium."),
  async execute(interaction, api) {
    if (!interaction.inCachedGuild()) {
      await interaction.reply({
        content: "❌ Please run this command in a server.",
        ephemeral: true,
      });
      return;
    }
    await interaction.deferReply({ ephemeral: true });

    try {
      const account = await api.me(interaction.user.id);

      if (!account.linked) {
        await interaction.editReply(
          "🔗 Your account is not linked yet. Use `/link` to get started.",
        );
        return;
      }

      const embed = new EmbedBuilder()
        .setTitle("👑 Premium")
        .setDescription(
          account.premium
            ? "You are a **Premium** PK Launcher member. Enjoy all the perks! 🎉"
            : "You do not have Premium yet. Support the launcher to unlock Premium perks.",
        )
        .setColor(account.premium ? "#F7B500" : "#6b7280");
      await interaction.editReply({ embeds: [embed] });
    } catch (error) {
      const message =
        error instanceof ApiError
          ? error.message
          : "Could not fetch your Premium status. Please try again later.";
      await interaction.editReply({ content: `❌ ${message}` });
    }
  },
};