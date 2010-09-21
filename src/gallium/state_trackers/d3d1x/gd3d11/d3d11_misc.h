#if API < 10
HRESULT D3D10CreateBlob(
	__in   SIZE_T NumBytes,
	__out  LPD3D10BLOB *ppBuffer
)
{
	void* data = malloc(NumBytes);
	if(!data)
		return E_OUTOFMEMORY;
	*ppBuffer = new GalliumD3DBlob(data, NumBytes);
	return S_OK;
}

LPCSTR STDMETHODCALLTYPE D3D10GetPixelShaderProfile(
	__in  ID3D10Device *pDevice
)
{
	return "ps_4_0";
}

LPCSTR STDMETHODCALLTYPE D3D10GetVertexShaderProfile(
	__in  ID3D10Device *pDevice
)
{
	return "vs_4_0";
}

LPCSTR STDMETHODCALLTYPE D3D10GetGeometryShaderProfile(
	__in  ID3D10Device *pDevice
)
{
	return "gs_4_0";
}

static HRESULT dxbc_assemble_as_blob(struct dxbc_chunk_header** chunks, unsigned num_chunks, ID3D10Blob** blob)
{
	std::pair<void*, size_t> p = dxbc_assemble(chunks, num_chunks);
	if(!p.first)
		return E_OUTOFMEMORY;
	*blob = return new GalliumD3DBlob(p.first, p.second);
	return S_OK;
}

HRESULT  D3D10GetInputSignatureBlob(
	__in   const void *pShaderBytecode,
	__in   SIZE_T BytecodeLength,
	__out  ID3D10Blob **ppSignatureBlob
)
{
	dxbc_chunk_signature* sig = dxbc_find_signature(pShaderBytecode, BytecodeLength, false);
	if(!sig)
		return E_FAIL;

	return dxbc_assemble_as_blob(&sig, 1, ppSignatureBlob);
}

HRESULT  D3D10GetOutputSignatureBlob(
	__in   const void *pShaderBytecode,
	__in   SIZE_T BytecodeLength,
	__out  ID3D10Blob **ppSignatureBlob
)
{
	dxbc_chunk_signature* sig = dxbc_find_signature(pShaderBytecode, BytecodeLength, true);
	if(!sig)
		return E_FAIL;

	return dxbc_assemble_as_blob(&sig, 1, ppSignatureBlob);
}

HRESULT  D3D10GetInputOutputSignatureBlob(
	__in   const void *pShaderBytecode,
	__in   SIZE_T BytecodeLength,
	__out  ID3D10Blob **ppSignatureBlob
)
{
	dxbc_chunk_signature* sigs[2];
	sigs[0] = dxbc_find_signature(pShaderBytecode, BytecodeLength, false);
	if(!sigs[0])
		return E_FAIL;
	sigs[1] = dxbc_find_signature(pShaderBytecode, BytecodeLength, true);
	if(!sigs[1])
		return E_FAIL;

	return dxbc_assemble_as_blob(&sigs, 2, ppSignatureBlob);
}

#endif
