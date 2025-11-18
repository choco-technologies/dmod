# Publishing the DMOD Manifest VSCode Extension

This document explains how to publish the DMOD Manifest VSCode extension to the Visual Studio Code Marketplace.

## Prerequisites

Before publishing, you need:

1. **Visual Studio Code Marketplace Account**
   - Create a publisher account at https://marketplace.visualstudio.com/manage
   - Publisher name: `ChocoTechnologiesDMOD`

2. **Personal Access Token (PAT)**
   - Create a PAT from Azure DevOps: https://dev.azure.com/
   - Required scopes: `Marketplace (Manage)`
   - Store the token securely

3. **GitHub Secret Configuration**
   - Add the PAT as a GitHub secret named `VSCE_PAT`
   - Go to: Repository Settings → Secrets and variables → Actions
   - Create a new repository secret

## Automated Publishing (Recommended)

The easiest way to publish a new version:

1. **Update the version** in `package.json`:
   ```bash
   cd tools/lib/dmod_manifest/vscode-dmm
   # Edit version in package.json
   ```

2. **Update CHANGELOG.md** with release notes:
   ```bash
   # Add new version section with changes
   ```

3. **Commit the changes**:
   ```bash
   git add package.json CHANGELOG.md
   git commit -m "Bump version to 1.2.0"
   git push
   ```

4. **Create and push a tag**:
   ```bash
   git tag vscode-dmm-v1.2.0
   git push origin vscode-dmm-v1.2.0
   ```

5. **Monitor the workflow**:
   - Go to: https://github.com/choco-technologies/dmod/actions
   - The "Publish VSCode Extension" workflow will automatically:
     - Package the extension
     - Publish it to the marketplace
     - Upload the .vsix as an artifact

## Manual Publishing

If you need to publish manually:

1. **Install vsce**:
   ```bash
   npm install -g @vscode/vsce
   ```

2. **Navigate to the extension directory**:
   ```bash
   cd tools/lib/dmod_manifest/vscode-dmm
   ```

3. **Login to vsce** (first time only):
   ```bash
   vsce login ChocoTechnologiesDMOD
   # Enter your Personal Access Token when prompted
   ```

4. **Publish with version bump**:
   ```bash
   # Patch version (1.1.0 -> 1.1.1)
   vsce publish patch
   
   # Minor version (1.1.0 -> 1.2.0)
   vsce publish minor
   
   # Major version (1.1.0 -> 2.0.0)
   vsce publish major
   
   # Or publish current version without bumping
   vsce publish
   ```

## Testing Before Publishing

Always test the extension before publishing:

1. **Package the extension**:
   ```bash
   cd tools/lib/dmod_manifest/vscode-dmm
   vsce package
   ```

2. **Install locally**:
   ```bash
   code --install-extension dmod-manifest-1.1.0.vsix
   ```

3. **Test the extension**:
   - Open a `.dmm` file
   - Verify syntax highlighting works correctly
   - Check that all features work as expected

4. **Uninstall after testing** (optional):
   ```bash
   code --uninstall-extension ChocoTechnologiesDMOD.dmod-manifest
   ```

## Troubleshooting

### Publishing Fails with "Access Denied"

- Verify the PAT has `Marketplace (Manage)` scope
- Ensure the PAT hasn't expired
- Check that the publisher name matches exactly

### Extension Not Appearing in Marketplace

- Wait 5-10 minutes after publishing (marketplace needs time to update)
- Clear browser cache
- Check the marketplace at: https://marketplace.visualstudio.com/items?itemName=ChocoTechnologiesDMOD.dmod-manifest

### GitHub Actions Workflow Fails

- Check that `VSCE_PAT` secret is set correctly
- Verify the tag format matches `vscode-dmm-v*`
- Review workflow logs for detailed error messages

## Version Numbering

Follow semantic versioning (semver):

- **Major** (X.0.0): Breaking changes or major new features
- **Minor** (1.X.0): New features, backward compatible
- **Patch** (1.1.X): Bug fixes, backward compatible

## Checklist

Before publishing a new version:

- [ ] Update version in `package.json`
- [ ] Update `CHANGELOG.md` with release notes
- [ ] Test the extension locally
- [ ] Commit and push changes
- [ ] Create and push git tag
- [ ] Verify GitHub Actions workflow succeeds
- [ ] Verify extension appears in marketplace

## Links

- **VS Code Marketplace**: https://marketplace.visualstudio.com/vscode
- **Extension Page**: https://marketplace.visualstudio.com/items?itemName=ChocoTechnologiesDMOD.dmod-manifest
- **Azure DevOps PAT**: https://dev.azure.com/
- **vsce Documentation**: https://code.visualstudio.com/api/working-with-extensions/publishing-extension
- **GitHub Actions**: https://github.com/choco-technologies/dmod/actions
