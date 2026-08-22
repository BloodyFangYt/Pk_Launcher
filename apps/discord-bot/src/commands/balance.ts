import { EmbedBuilder, SlashCommandBuilder } from "discord.js";
import { ApiError } from "../backend/client.js";
import type { Command } from "../types.js";

/** /balance — your current PK Launcher account balance. */
export const balance: Command = {
  data: new SlashCommandBuilder()
    .setName("balance")
    .setDescription("View your current PK Launcher balance."),
  async execute(interaction, ctx) {
    if (!interaction.inCachedGuild()) {
      await interaction.reply({
        content: "❌ Please run this command in a server.",
        ephemeral: true,
      });
      return;
    }
    await interaction.deferReply({ ephemeral: true });

    try {
      const account = await ctx.me(interaction.user.id);

      if (!account.linked) {
        await interaction.editReply(
          "🔗 Your Discord is not linked yet. Use `/link` with a code from the launcher.",
        );
        return;
      }

      const amount = Number.isFinite(account.balance)
        ? account.balance.toLocaleString()
        : "0";
      const embed = new EmbedBuilder()
        .setTitle("💳 Balance")
        .setDescription(`${amount} PK coins`)
        .setColor("#FFB800");
      await interaction.editReply({ embeds: [embed] });
    } catch (error) {
      const message =
        error instanceof ApiError
          ? error.message
          : "Could not fetch your balance. Please try again later.";
      await interaction.editReply({ content: `❌ ${message}` });
    }
  },
};