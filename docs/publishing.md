# Publishing `lc-ns-kmp` to PyPI

## One-time: Trusted Publisher on pypi.org

1. Log in at https://pypi.org
2. Open **Your account → Publishing**  
   https://pypi.org/manage/account/publishing/
3. Under **Add a new pending publisher**, enter **exactly**:

| Field | Value |
|---|---|
| PyPI Project Name | `lc-ns-kmp` |
| Owner | `abhishekpadalkar` |
| Repository name | `lc-ns-kmp` |
| Workflow name | `wheels.yml` |
| Environment name | `pypi` |

4. On GitHub: **Settings → Environments → New environment** named `pypi`
5. Click **Add** on PyPI

Do **not** use `Wheels` or a full path like `.github/workflows/wheels.yml` for the workflow name.

## Publish a release

```bash
# bump version in pyproject.toml / CMakeLists.txt if needed
git tag v0.1.x
git push origin v0.1.x
```

Watch **Actions → Wheels**. The **Publish to PyPI** job runs on version tags.

Package page: https://pypi.org/project/lc-ns-kmp/

## TestPyPI (optional)

Same form on https://test.pypi.org with Environment name `testpypi`, plus a GitHub environment `testpypi`.  
Then: Actions → Wheels → **Run workflow** (manual).
