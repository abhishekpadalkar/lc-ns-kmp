# Publishing `lc-ns-kmp` to PyPI

## One-time setup (Trusted Publisher)

No API tokens needed if you use OpenID Connect.

### TestPyPI

1. Create an account on https://test.pypi.org
2. Go to **Publishing** → **Add a new pending publisher**
3. Fill in:
   - PyPI project name: `lc-ns-kmp`
   - Owner: `abhishekpadalkar`
   - Repository: `lc-ns-kmp`
   - Workflow: `wheels.yml`
   - Environment: `testpypi`
4. In GitHub: **Settings → Environments → New environment** named `testpypi`

### PyPI

Same steps on https://pypi.org with environment name `pypi`.

You can create the project on first upload; the pending publisher must match.

## Dry-run (TestPyPI)

1. Push this repo to GitHub.
2. Actions → **Wheels** → **Run workflow**.
3. After success:

```bash
pip install -i https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ lc-ns-kmp==0.1.0
```

## Production release

1. Bump `version` in `pyproject.toml` (and `project(... VERSION ...)` in `CMakeLists.txt` if you keep them in sync).
2. Commit, then:

```bash
git tag v0.1.0
git push origin v0.1.0
```

3. The **Wheels** workflow builds Linux/macOS wheels + sdist and publishes to PyPI.

## Local smoke build (optional)

```bash
python3 -m venv .venv && source .venv/bin/activate
pip install build
python -m build --wheel
pip install dist/*.whl
python -c "from lc_ns_kmp import LC_NS_KMP; print(LC_NS_KMP(2, 8))"
```

Wheel builds fetch Eigen/jsoncpp/PIQP via CMake (`LC_NS_KMP_FETCH_DEPS`) and statically link PIQP into `_core`.
