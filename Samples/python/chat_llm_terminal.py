#!/usr/bin/env python3
"""
Minimal terminal chat against Azure OpenAI or OpenAI — same config sources as PyViewer:
  - Non-empty env vars (see below)
  - Optional .env (cwd, OPENVISUS_ENV_FILE, ~/.openvisus.env, next to this script)
  - ~/.openvisus/agentic.json (from Viewer "Agent Settings")

Usage:
  python chat_llm_terminal.py
  python chat_llm_terminal.py --once "Say hello in one sentence."

Azure: AZURE_OPENAI_ENDPOINT + AZURE_OPENAI_API_KEY
       (+ optional AZURE_OPENAI_DEPLOYMENT_NAME, AZURE_OPENAI_API_VERSION)
OpenAI: OPENAI_API_KEY (+ optional OPENAI_MODEL)

Optional: OPENVISUS_AGENTIC_JSON=full path to agentic.json
          OPENVISUS_HOME=profile dir (if ~ is wrong for this process)

No pip packages required (stdlib only). Implementation: OpenVisus.agent_llm
"""

from __future__ import annotations

import argparse
import os
import sys

_this_dir = os.path.dirname(os.path.abspath(__file__))
_swig = os.path.normpath(os.path.join(_this_dir, "..", "..", "Libs", "swig"))


def _load_agent_llm():
	try:
		from OpenVisus import agent_llm

		return agent_llm
	except ImportError:
		pass
	import importlib.util

	_path = os.path.join(_swig, "agent_llm.py")
	if not os.path.isfile(_path):
		raise ImportError(
			"Could not import OpenVisus.agent_llm and %s is missing. "
			"Install OpenVisus or run from the repository checkout." % _path
		)
	spec = importlib.util.spec_from_file_location("openvisus_agent_llm", _path)
	mod = importlib.util.module_from_spec(spec)
	assert spec.loader is not None
	spec.loader.exec_module(mod)
	return mod


agent_llm = _load_agent_llm()


def main() -> int:
	parser = argparse.ArgumentParser(description="Terminal LLM chat (Azure OpenAI or OpenAI)")
	parser.add_argument("--once", metavar="TEXT", help="Single user message, then exit")
	parser.add_argument("--system", default="You are a helpful assistant. Be concise.", help="System prompt")
	args = parser.parse_args()

	agent_llm.load_dotenv()
	v = agent_llm.effective_config()
	ap = agent_llm.agentic_path()

	if v["azure_key"] and v["azure_endpoint"]:
		mode = "Azure OpenAI"
	elif v["openai_key"]:
		mode = "OpenAI"
	else:
		print(
			"No credentials: set AZURE_OPENAI_ENDPOINT+AZURE_OPENAI_API_KEY or OPENAI_API_KEY, "
			"or fill ~/.openvisus/agentic.json (Agent Settings)."
		)
		if ap:
			print(f"agentic.json path: {ap} (exists={os.path.isfile(ap)})")
		return 1

	print(f"Backend: {mode}")
	if ap and os.path.isfile(ap):
		print(f"Using keys from: {ap} (where env not set)")

	messages: list[dict] = [{"role": "system", "content": args.system}]

	def turn(user_text: str) -> str:
		messages.append({"role": "user", "content": user_text})
		try:
			reply = agent_llm.chat_completion(v, messages)
		except Exception as e:
			raise SystemExit(str(e)) from e
		messages.append({"role": "assistant", "content": reply})
		return reply

	if args.once is not None:
		print(turn(args.once.strip()))
		return 0

	print("Type a message (empty line or Ctrl+Z+Enter / Ctrl+D to quit).")
	while True:
		try:
			line = input("You: ").strip()
		except EOFError:
			break
		if not line:
			break
		try:
			out = turn(line)
		except SystemExit as e:
			print(e, file=sys.stderr)
			return 1
		print("Assistant:", out)
		print()
	return 0


if __name__ == "__main__":
	sys.exit(main())
