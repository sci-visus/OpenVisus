"""
Azure OpenAI / OpenAI for Visus agent chat (shared with chat_llm_terminal).
Reads ~/.openvisus/agentic.json (and env) like chat_llm_terminal.py.
"""

from __future__ import annotations

import json
import os
import re
import sys
import urllib.error
import urllib.parse
import urllib.request

# ---------------------------------------------------------------------------
# Config (aligned with Samples/python/chat_llm_terminal.py)


def _env_nonempty(key: str) -> bool:
	v = os.environ.get(key)
	return v is not None and str(v).strip() != ""


def _user_home() -> str:
	override = os.environ.get("OPENVISUS_HOME")
	if override and str(override).strip():
		return os.path.normpath(str(override).strip().strip('"').strip("'"))
	try:
		from pathlib import Path

		h = Path.home()
		if h is not None:
			s = str(h)
			if s and s not in (".", "~"):
				return os.path.normpath(s)
	except Exception:
		pass
	for key in ("USERPROFILE", "HOME"):
		v = os.environ.get(key)
		if v and str(v).strip():
			return os.path.normpath(str(v).strip().strip('"').strip("'"))
	h = os.path.expanduser("~")
	if h and h != "~":
		return os.path.normpath(h)
	return ""


def _load_dotenv_file(path: str) -> None:
	try:
		with open(path, encoding="utf-8-sig") as f:
			for line in f:
				line = line.strip()
				if not line or line.startswith("#"):
					continue
				if line.startswith("export "):
					line = line[7:].strip()
				if "=" not in line:
					continue
				key, _, val = line.partition("=")
				key = key.strip()
				if not key:
					continue
				val = val.strip()
				if len(val) >= 2 and val[0] == val[-1] and val[0] in "\"'":
					val = val[1:-1]
				existing = os.environ.get(key)
				if existing is not None and str(existing).strip() != "":
					continue
				os.environ[key] = val
	except OSError:
		pass


def load_dotenv() -> None:
	seen: set[str] = set()
	paths: list[str] = []
	extra = os.environ.get("OPENVISUS_ENV_FILE") or os.environ.get("OPENAI_DOTENV")
	if extra:
		paths.append(extra.strip().strip('"').strip("'"))
	paths.append(os.path.join(os.getcwd(), ".env"))
	try:
		here = os.path.dirname(os.path.abspath(__file__))
		paths.append(os.path.join(here, ".env"))
	except Exception:
		pass
	home = _user_home()
	if home:
		paths.append(os.path.join(home, ".openvisus.env"))
	for p in paths:
		if not p or p in seen:
			continue
		seen.add(p)
		if os.path.isfile(p):
			_load_dotenv_file(p)


def agentic_path() -> str:
	explicit = os.environ.get("OPENVISUS_AGENTIC_JSON")
	if explicit and str(explicit).strip():
		return os.path.normpath(str(explicit).strip().strip('"').strip("'"))
	home = _user_home()
	return os.path.join(home, ".openvisus", "agentic.json") if home else ""


def load_agentic() -> dict:
	p = agentic_path()
	if not p or not os.path.isfile(p):
		return {}
	try:
		with open(p, encoding="utf-8") as f:
			data = json.load(f)
		return data if isinstance(data, dict) else {}
	except (OSError, json.JSONDecodeError):
		return {}


def pick(env_key: str, store: dict, json_key: str, default: str | None = "") -> str:
	if _env_nonempty(env_key):
		return os.environ.get(env_key, "").strip()
	v = store.get(json_key)
	if v is None:
		return str(default).strip() if default is not None else ""
	s = str(v).strip()
	if not s and default is not None and str(default).strip():
		return str(default).strip()
	return s


def pick_bool(store: dict, json_key: str, default: bool = True) -> bool:
	v = store.get(json_key)
	if v is None:
		return default
	if isinstance(v, bool):
		return v
	s = str(v).strip().lower()
	if s in ("0", "false", "no", "off"):
		return False
	if s in ("1", "true", "yes", "on"):
		return True
	return default


def effective_config() -> dict:
	st = load_agentic()
	return {
		"azure_endpoint": pick("AZURE_OPENAI_ENDPOINT", st, "azure_openai_endpoint"),
		"azure_key": pick("AZURE_OPENAI_API_KEY", st, "azure_openai_api_key"),
		"azure_deployment": pick(
			"AZURE_OPENAI_DEPLOYMENT_NAME", st, "azure_openai_deployment_name", "gpt-4o"
		),
		"azure_version": pick(
			"AZURE_OPENAI_API_VERSION", st, "azure_openai_api_version", "2024-08-01-preview"
		),
		"openai_key": pick("OPENAI_API_KEY", st, "openai_api_key"),
		"openai_model": pick("OPENAI_MODEL", st, "openai_model", "gpt-4o"),
		"agent_llm_enabled": pick_bool(st, "agent_chat_llm_enabled", True),
	}


def azure_url(v: dict) -> str:
	ep = v["azure_endpoint"].rstrip("/")
	dep = v["azure_deployment"] or "gpt-4o"
	ver = v["azure_version"] or "2024-08-01-preview"
	path = f"/openai/deployments/{urllib.parse.quote(dep, safe='')}/chat/completions"
	q = urllib.parse.urlencode({"api-version": ver})
	return f"{ep}{path}?{q}"


def chat_completion(v: dict, messages: list[dict]) -> str:
	body_obj: dict = {"messages": messages, "temperature": 0.3}
	use_azure = bool(v["azure_key"] and v["azure_endpoint"])
	if use_azure:
		body_obj["max_completion_tokens"] = 2048
		url = azure_url(v)
		body = json.dumps(body_obj).encode("utf-8")
		req = urllib.request.Request(url, data=body, method="POST")
		req.add_header("Content-Type", "application/json")
		req.add_header("api-key", v["azure_key"])
	else:
		url = "https://api.openai.com/v1/chat/completions"
		body_obj["model"] = v["openai_model"] or "gpt-4o"
		body = json.dumps(body_obj).encode("utf-8")
		req = urllib.request.Request(url, data=body, method="POST")
		req.add_header("Content-Type", "application/json")
		req.add_header("Authorization", "Bearer " + v["openai_key"])

	with urllib.request.urlopen(req, timeout=120) as resp:
		raw = resp.read().decode("utf-8", errors="replace")

	data = json.loads(raw)
	choices = data.get("choices") or []
	if not choices:
		raise RuntimeError("empty choices: " + raw[:800])
	msg = choices[0].get("message") or {}
	content = msg.get("content")
	if content is None:
		raise RuntimeError("no message content: " + raw[:800])
	if isinstance(content, list):
		parts = []
		for part in content:
			if isinstance(part, dict) and part.get("type") == "text":
				parts.append(part.get("text") or "")
			elif isinstance(part, str):
				parts.append(part)
		content = "".join(parts)
	return content if isinstance(content, str) else str(content)


def has_llm_credentials(v: dict) -> bool:
	if v.get("azure_key") and v.get("azure_endpoint"):
		return True
	if v.get("openai_key"):
		return True
	return False


def use_llm_for_agent_chat() -> bool:
	load_dotenv()
	v = effective_config()
	if not v.get("agent_llm_enabled", True):
		if _env_nonempty("OPENVISUS_AGENT_LLM_DEBUG"):
			print("agent_llm: disabled (agent_chat_llm_enabled is false in agentic.json)", file=sys.stderr)
		return False
	ok = has_llm_credentials(v)
	if _env_nonempty("OPENVISUS_AGENT_LLM_DEBUG"):
		ap = agentic_path()
		print(
			"agent_llm: agentic.json=",
			ap,
			"exists=",
			os.path.isfile(ap) if ap else False,
			file=sys.stderr,
		)
		print(
			"agent_llm: has endpoint/key/openai=",
			bool(v.get("azure_endpoint")),
			bool(v.get("azure_key")),
			bool(v.get("openai_key")),
			"use_llm=",
			ok,
			file=sys.stderr,
		)
	return ok


def viewer_run_builtin(viewer, msg: str) -> None:
	"""Execute C++ viewer builtin (commands + XML). Prefer PyViewer._call_viewer_builtin_chat."""
	fn = getattr(viewer, "_call_viewer_builtin_chat", None)
	if callable(fn):
		fn(msg)
		return
	for name in ("agentChatRunBuiltinMessage", "agentChatProcessBuiltinMessage"):
		ap = getattr(viewer, name, None)
		if callable(ap):
			ap(msg)
			return
	try:
		from OpenVisus import Viewer as _V

		for name in ("agentChatRunBuiltinMessage", "agentChatProcessBuiltinMessage"):
			ap = getattr(_V, name, None)
			if callable(ap):
				ap(viewer, msg)
				return
	except Exception:
		pass
	raise RuntimeError(
		"Cannot run viewer builtin chat: rebuild VisusGuiPy (agentChatRunBuiltinMessage in VisusGuiPy.i), or use PyViewer."
	)


def viewer_append_line(viewer, text: str) -> None:
	"""Route to PyViewer._append_agent_chat_line first so replies go to the wired chat QTextEdit, not only printInfo."""
	ap = getattr(viewer, "_append_agent_chat_line", None)
	if callable(ap):
		ap(text)
		return
	fn = getattr(viewer, "appendAgentChatLine", None)
	if callable(fn):
		fn(text)
		return
	try:
		from OpenVisus import Viewer as _V

		fn = getattr(_V, "appendAgentChatLine", None)
		if callable(fn):
			fn(viewer, text)
			return
	except Exception:
		pass
	try:
		viewer.printInfo(text)
	except Exception:
		print(text, file=sys.stderr)


def prompt_configure_llm(viewer) -> None:
	viewer_append_line(
		'Agent chat uses the LLM only. Configure Azure OpenAI or OpenAI in Agent Settings (or .env), '
		'enable "Use LLM", then restart. For legacy viewer commands without the LLM, start a line with /local '
		"(example: /local help)."
	)


AGENT_SYSTEM_PROMPT = """You are the assistant for the Visus scientific visualization viewer.
The user message may be a question OR a request to manipulate the scene.

Rules:
- If the user wants an action that matches a command below, use mode "commands" or "both" with EXACT command line(s). Never claim a capability is missing if it appears here.
- One command per string in "lines" (same as the viewer's built-in agent; the user can type "help" there for the canonical list).

JSON only (no markdown fences). Schema:
{"mode":"answer","text":"..."} — explanations or Q&A only; no commands.
{"mode":"commands","lines":["cmd1","cmd2",...]} — run each line in order.
{"mode":"both","text":"...","lines":["cmd1",...]} — short reply plus commands.

--- Time-varying data ---
- Natural language: phrases like "play over time" start timestep playback; the word "stop" (or "time stop") stops it.
- time play | time stop — explicit commands (same behavior as above).

--- Files / scene ---
- open <url or path> — open dataset (.idx etc.), scene XML, or .config (same as File → Open).

--- Atlantis (mod_visus server at SCI Utah) ---
When the user asks to open a dataset on Atlantis, or mentions Atlantis / atlantis.sci.utah.edu, build the URL from the dataset id (not a local path):
- Pattern: http://atlantis.sci.utah.edu/mod_visus?dataset=<DATASET_NAME>
- Put the dataset identifier in the query parameter only; the host and path are fixed as above.
- Example: dataset id nex-gddp-cmip6 → {"mode":"commands","lines":["open http://atlantis.sci.utah.edu/mod_visus?dataset=nex-gddp-cmip6"]}
- If the user gives only a name (e.g. "open nex-gddp-cmip6 on Atlantis"), infer Atlantis and use that URL form.
- For other remote servers, use the URL the user provides; do not assume Atlantis unless they say Atlantis / that host.

--- ADD menu (needs a scene; many subcommands need a dataset in the scene) ---
- add group [name] — default name Group if omitted.
- add transform — ModelView under current selection.
- add insert transform — insert ModelView (requires a non-root node selected).
- add slice x | add slice y | add slice z — axis can also be 0, 1, 2 (needs DatasetNode).
- add volume — needs DatasetNode.
- add isocontour [value] — aliases: add iso, add isosurface (optional isovalue; needs DatasetNode).
- add kdquery — alias: add kd_query (needs DatasetNode).
- add render — needs selection with array output.
- add kdrender — needs KdQueryMode selection.
- add scripting — aliases: add script (adds a ScriptingNode).
- add statistics — aliases: add stats.

Shorthand (no "add" prefix): slice x | slice y | slice z — same as add slice x|y|z.

--- Dataflow / processing ---
- refresh [uuid] — refresh selected node, or node with given UUID.
- refreshall — refresh entire dataflow.
- drop — cancel data processing (toolbar "Drop processing"). Do not confuse with time "stop".

--- Camera / view ---
- fit — best camera fit to content.
- camera x | camera y | camera z | camera fit — axis-aligned or fit view.
- zoom in | zoom out — also synonyms: +, -, closer, farther (ortho: scale; look-at: field of view).
- mirror x | mirror y — orthographic camera only.
- rotate +x | -x | +y | -y | +z | -z — 5° steps (look-at camera).
- rotate <deg> <axis> — e.g. rotate 10 x, rotate +15 y, rotate -20 z, rotate x 10 (look-at only; ortho ignores rotation).

--- Images / automation ---
- snapshot [canvas|window] [file.png] — save PNG; path optional (auto if omitted).
- autorefresh on [msec] | autorefresh off — periodic refresh.

--- Selection / tree ---
- nodes — list nodes (name, type, UUID) for select/hide/show.
- select <uuid> | deselect
- show <uuid> | hide <uuid>

--- Palette / colors ---
- palette <name> — set palette on selected PaletteNode, or all PaletteNodes if none selected.
- color <name> — alias for palette (natural phrasing), e.g. color red.
- Common color words map smartly: red->Reds, blue->Blues, green->Greens, gray/grey->Greys, orange->Oranges, purple->Purples.
- Typical palette names include Reds, Blues, Greens, Greys, Viridis, Turbo, Hot1.

--- Node fields (when applicable) ---
- field <name> — set field on FieldNode if present.
- script <code> — set ScriptingNode code if present (this is NOT the same as "add scripting", which adds a node).

--- History ---
- undo | redo

--- Help / meta ---
- help | ? | commands — print full command help in the viewer.

--- Advanced ---
- A line starting with "<" is XML/StringTree — forwarded to Viewer::execute (use valid UUIDs; can assert on error).

Examples: open D:/data/volume.idx — open http://atlantis.sci.utah.edu/mod_visus?dataset=nex-gddp-cmip6 — add slice x — palette Reds — color red — zoom in — rotate 10 x — fit

Prefer mode "answer" for pure information. Use "commands" or "both" for actions.
If unsure what exists, use {"mode":"commands","lines":["help"]} instead of guessing that something is unsupported.
"""


def _extract_json_object(text: str) -> dict | None:
	t = text.strip()
	m = re.search(r"\{[\s\S]*\}\s*$", t)
	if not m:
		m = re.search(r"\{[\s\S]*\}", t)
	if not m:
		return None
	try:
		obj = json.loads(m.group(0))
		return obj if isinstance(obj, dict) else None
	except json.JSONDecodeError:
		return None


def handle_agent_chat(viewer, message: str) -> None:
	"""LLM turn: interpret user message, then run builtin agent or reply."""
	msg = (message or "").strip()
	if not msg:
		viewer_run_builtin(viewer, message)
		return

	if msg.lower().startswith("/local "):
		viewer_run_builtin(viewer, msg[7:].lstrip())
		return

	if msg.startswith("<"):
		viewer_run_builtin(viewer, message)
		return

	load_dotenv()
	v = effective_config()
	if not has_llm_credentials(v):
		prompt_configure_llm(viewer)
		return

	hist = getattr(viewer, "_agent_llm_messages", None)
	if not isinstance(hist, list):
		hist = []
		setattr(viewer, "_agent_llm_messages", hist)

	msgs: list[dict] = [{"role": "system", "content": AGENT_SYSTEM_PROMPT}]
	for turn in hist[-16:]:
		if isinstance(turn, dict) and turn.get("role") in ("user", "assistant") and "content" in turn:
			msgs.append({"role": turn["role"], "content": str(turn["content"])})
	msgs.append({"role": "user", "content": msg})

	try:
		raw = chat_completion(v, msgs)
	except urllib.error.HTTPError as e:
		err = ""
		try:
			err = e.read().decode("utf-8", errors="replace")[:1500]
		except Exception:
			pass
		viewer_append_line(viewer, f"LLM HTTP {e.code}: {err or e.reason}")
		return
	except Exception as e:
		viewer_append_line(viewer, f"LLM error: {e}")
		return

	plan = _extract_json_object(raw)
	if not plan:
		viewer_append_line(viewer, raw.strip() if raw.strip() else "(empty model reply)")
		hist.append({"role": "user", "content": msg})
		hist.append({"role": "assistant", "content": raw.strip()})
		return

	mode = str(plan.get("mode", "answer")).lower()

	hist.append({"role": "user", "content": msg})
	assistant_text = ""

	if mode == "answer":
		assistant_text = str(plan.get("text") or raw).strip()
		if assistant_text:
			viewer_append_line(viewer, assistant_text)
	elif mode == "commands":
		lines = plan.get("lines")
		if isinstance(lines, str):
			lines = [lines]
		if isinstance(lines, list):
			for line in lines:
				line = str(line).strip()
				if line:
					viewer_run_builtin(viewer, line)
		assistant_text = "(commands)"
	elif mode == "both":
		t = plan.get("text")
		if t and str(t).strip():
			assistant_text = str(t).strip()
			viewer_append_line(viewer, assistant_text)
		lines = plan.get("lines")
		if isinstance(lines, str):
			lines = [lines]
		if isinstance(lines, list):
			for line in lines:
				line = str(line).strip()
				if line:
					viewer_run_builtin(viewer, line)
	else:
		assistant_text = json.dumps(plan, ensure_ascii=False)
		viewer_append_line(viewer, assistant_text)

	hist.append({"role": "assistant", "content": assistant_text or raw[:2000]})
	if len(hist) > 40:
		hist[:] = hist[-40:]
