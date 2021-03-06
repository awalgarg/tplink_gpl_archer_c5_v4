
/* Python wrapper functions auto-generated by pidl */
#include <Python.h>
#include "includes.h"
#include <pytalloc.h>
#include "librpc/rpc/pyrpc.h"
#include "librpc/rpc/pyrpc_util.h"
#include "autoconf/librpc/gen_ndr/ndr_remact.h"
#include "autoconf/librpc/gen_ndr/ndr_remact_c.h"

#include "librpc/gen_ndr/misc.h"
#include "librpc/gen_ndr/orpc.h"
staticforward PyTypeObject IRemoteActivation_InterfaceType;

void initremact(void);static PyTypeObject *MInterfacePointer_Type;
static PyTypeObject *ORPCTHAT_Type;
static PyTypeObject *ORPCTHIS_Type;
static PyTypeObject *COMVERSION_Type;
static PyTypeObject *DUALSTRINGARRAY_Type;
static PyTypeObject *GUID_Type;
static PyTypeObject *ClientConnection_Type;

static bool pack_py_RemoteActivation_args_in(PyObject *args, PyObject *kwargs, struct RemoteActivation *r)
{
	PyObject *py_this_object;
	PyObject *py_Clsid;
	PyObject *py_pwszObjectName;
	PyObject *py_pObjectStorage;
	PyObject *py_ClientImpLevel;
	PyObject *py_Mode;
	PyObject *py_pIIDs;
	PyObject *py_protseq;
	const char *kwnames[] = {
		"this_object", "Clsid", "pwszObjectName", "pObjectStorage", "ClientImpLevel", "Mode", "pIIDs", "protseq", NULL
	};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOOOOOOO:RemoteActivation", discard_const_p(char *, kwnames), &py_this_object, &py_Clsid, &py_pwszObjectName, &py_pObjectStorage, &py_ClientImpLevel, &py_Mode, &py_pIIDs, &py_protseq)) {
		return false;
	}

	PY_CHECK_TYPE(ORPCTHIS_Type, py_this_object, return false;);
	if (talloc_reference(r, pytalloc_get_mem_ctx(py_this_object)) == NULL) {
		PyErr_NoMemory();
		return false;
	}
	r->in.this_object = *(struct ORPCTHIS *)pytalloc_get_ptr(py_this_object);
	PY_CHECK_TYPE(GUID_Type, py_Clsid, return false;);
	if (talloc_reference(r, pytalloc_get_mem_ctx(py_Clsid)) == NULL) {
		PyErr_NoMemory();
		return false;
	}
	r->in.Clsid = *(struct GUID *)pytalloc_get_ptr(py_Clsid);
	r->in.pwszObjectName = talloc_ptrtype(r, r->in.pwszObjectName);
	if (PyUnicode_Check(py_pwszObjectName)) {
		r->in.pwszObjectName = PyString_AS_STRING(PyUnicode_AsEncodedString(py_pwszObjectName, "utf-8", "ignore"));
	} else if (PyString_Check(py_pwszObjectName)) {
		r->in.pwszObjectName = PyString_AS_STRING(py_pwszObjectName);
	} else {
		PyErr_Format(PyExc_TypeError, "Expected string or unicode object, got %s", Py_TYPE(py_pwszObjectName)->tp_name);
		return false;
	}
	r->in.pObjectStorage = talloc_ptrtype(r, r->in.pObjectStorage);
	PY_CHECK_TYPE(MInterfacePointer_Type, py_pObjectStorage, return false;);
	if (talloc_reference(r, pytalloc_get_mem_ctx(py_pObjectStorage)) == NULL) {
		PyErr_NoMemory();
		return false;
	}
	r->in.pObjectStorage = (struct MInterfacePointer *)pytalloc_get_ptr(py_pObjectStorage);
	PY_CHECK_TYPE(&PyInt_Type, py_ClientImpLevel, return false;);
	r->in.ClientImpLevel = PyInt_AsLong(py_ClientImpLevel);
	PY_CHECK_TYPE(&PyInt_Type, py_Mode, return false;);
	r->in.Mode = PyInt_AsLong(py_Mode);
	PY_CHECK_TYPE(&PyList_Type, py_pIIDs, return false;);
	r->in.Interfaces = PyList_GET_SIZE(py_pIIDs);
	r->in.pIIDs = talloc_ptrtype(r, r->in.pIIDs);
	PY_CHECK_TYPE(&PyList_Type, py_pIIDs, return false;);
	{
		int pIIDs_cntr_1;
		r->in.pIIDs = talloc_array_ptrtype(r, r->in.pIIDs, PyList_GET_SIZE(py_pIIDs));
		if (!r->in.pIIDs) { return false;; }
		talloc_set_name_const(r->in.pIIDs, "ARRAY: r->in.pIIDs");
		for (pIIDs_cntr_1 = 0; pIIDs_cntr_1 < PyList_GET_SIZE(py_pIIDs); pIIDs_cntr_1++) {
			PY_CHECK_TYPE(GUID_Type, PyList_GET_ITEM(py_pIIDs, pIIDs_cntr_1), return false;);
			if (talloc_reference(r->in.pIIDs, pytalloc_get_mem_ctx(PyList_GET_ITEM(py_pIIDs, pIIDs_cntr_1))) == NULL) {
				PyErr_NoMemory();
				return false;
			}
			r->in.pIIDs[pIIDs_cntr_1] = *(struct GUID *)pytalloc_get_ptr(PyList_GET_ITEM(py_pIIDs, pIIDs_cntr_1));
		}
	}
	PY_CHECK_TYPE(&PyList_Type, py_protseq, return false;);
	r->in.num_protseqs = PyList_GET_SIZE(py_protseq);
	PY_CHECK_TYPE(&PyList_Type, py_protseq, return false;);
	{
		int protseq_cntr_0;
		r->in.protseq = talloc_array_ptrtype(r, r->in.protseq, PyList_GET_SIZE(py_protseq));
		if (!r->in.protseq) { return false;; }
		talloc_set_name_const(r->in.protseq, "ARRAY: r->in.protseq");
		for (protseq_cntr_0 = 0; protseq_cntr_0 < PyList_GET_SIZE(py_protseq); protseq_cntr_0++) {
			PY_CHECK_TYPE(&PyInt_Type, PyList_GET_ITEM(py_protseq, protseq_cntr_0), return false;);
			r->in.protseq[protseq_cntr_0] = PyInt_AsLong(PyList_GET_ITEM(py_protseq, protseq_cntr_0));
		}
	}
	return true;
}

static PyObject *unpack_py_RemoteActivation_args_out(struct RemoteActivation *r)
{
	PyObject *result;
	PyObject *py_that;
	PyObject *py_pOxid;
	PyObject *py_pdsaOxidBindings;
	PyObject *py_ipidRemUnknown;
	PyObject *py_AuthnHint;
	PyObject *py_ServerVersion;
	PyObject *py_hr;
	PyObject *py_ifaces;
	PyObject *py_results;
	result = PyTuple_New(9);
	py_that = pytalloc_reference_ex(ORPCTHAT_Type, r->out.that, r->out.that);
	PyTuple_SetItem(result, 0, py_that);
	py_pOxid = PyLong_FromLongLong(*r->out.pOxid);
	PyTuple_SetItem(result, 1, py_pOxid);
	py_pdsaOxidBindings = pytalloc_reference_ex(DUALSTRINGARRAY_Type, r->out.pdsaOxidBindings, r->out.pdsaOxidBindings);
	PyTuple_SetItem(result, 2, py_pdsaOxidBindings);
	py_ipidRemUnknown = pytalloc_reference_ex(GUID_Type, r->out.ipidRemUnknown, r->out.ipidRemUnknown);
	PyTuple_SetItem(result, 3, py_ipidRemUnknown);
	py_AuthnHint = PyInt_FromLong(*r->out.AuthnHint);
	PyTuple_SetItem(result, 4, py_AuthnHint);
	py_ServerVersion = pytalloc_reference_ex(COMVERSION_Type, r->out.ServerVersion, r->out.ServerVersion);
	PyTuple_SetItem(result, 5, py_ServerVersion);
	py_hr = PyErr_FromWERROR(*r->out.hr);
	PyTuple_SetItem(result, 6, py_hr);
	py_ifaces = PyList_New(r->in.Interfaces);
	if (py_ifaces == NULL) {
		return NULL;
	}
	{
		int ifaces_cntr_0;
		for (ifaces_cntr_0 = 0; ifaces_cntr_0 < r->in.Interfaces; ifaces_cntr_0++) {
			PyObject *py_ifaces_0;
			py_ifaces_0 = pytalloc_reference_ex(MInterfacePointer_Type, r->out.ifaces[ifaces_cntr_0], r->out.ifaces[ifaces_cntr_0]);
			PyList_SetItem(py_ifaces, ifaces_cntr_0, py_ifaces_0);
		}
	}
	PyTuple_SetItem(result, 7, py_ifaces);
	py_results = PyList_New(r->in.Interfaces);
	if (py_results == NULL) {
		return NULL;
	}
	{
		int results_cntr_0;
		for (results_cntr_0 = 0; results_cntr_0 < r->in.Interfaces; results_cntr_0++) {
			PyObject *py_results_0;
			py_results_0 = PyErr_FromWERROR(r->out.results[results_cntr_0]);
			PyList_SetItem(py_results, results_cntr_0, py_results_0);
		}
	}
	PyTuple_SetItem(result, 8, py_results);
	if (!W_ERROR_IS_OK(r->out.result)) {
		PyErr_SetWERROR(r->out.result);
		return NULL;
	}

	return result;
}

const struct PyNdrRpcMethodDef py_ndr_IRemoteActivation_methods[] = {
	{ "RemoteActivation", "S.RemoteActivation(this_object, Clsid, pwszObjectName, pObjectStorage, ClientImpLevel, Mode, pIIDs, protseq) -> (that, pOxid, pdsaOxidBindings, ipidRemUnknown, AuthnHint, ServerVersion, hr, ifaces, results)", (py_dcerpc_call_fn)dcerpc_RemoteActivation_r, (py_data_pack_fn)pack_py_RemoteActivation_args_in, (py_data_unpack_fn)unpack_py_RemoteActivation_args_out, 0, &ndr_table_IRemoteActivation },
	{ NULL }
};

static PyObject *interface_IRemoteActivation_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	return py_dcerpc_interface_init_helper(type, args, kwargs, &ndr_table_IRemoteActivation);
}

static PyTypeObject IRemoteActivation_InterfaceType = {
	PyObject_HEAD_INIT(NULL) 0,
	.tp_name = "remact.IRemoteActivation",
	.tp_basicsize = sizeof(dcerpc_InterfaceObject),
	.tp_doc = "IRemoteActivation(binding, lp_ctx=None, credentials=None) -> connection\n"
"\n"
"binding should be a DCE/RPC binding string (for example: ncacn_ip_tcp:127.0.0.1)\n"
"lp_ctx should be a path to a smb.conf file or a param.LoadParm object\n"
"credentials should be a credentials.Credentials object.\n\n",
	.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_new = interface_IRemoteActivation_new,
};

static PyMethodDef remact_methods[] = {
	{ NULL, NULL, 0, NULL }
};

void initremact(void)
{
	PyObject *m;
	PyObject *dep_samba_dcerpc_misc;
	PyObject *dep_samba_dcerpc_orpc;
	PyObject *dep_samba_dcerpc_base;

	dep_samba_dcerpc_misc = PyImport_ImportModule("samba.dcerpc.misc");
	if (dep_samba_dcerpc_misc == NULL)
		return;

	dep_samba_dcerpc_orpc = PyImport_ImportModule("samba.dcerpc.orpc");
	if (dep_samba_dcerpc_orpc == NULL)
		return;

	dep_samba_dcerpc_base = PyImport_ImportModule("samba.dcerpc.base");
	if (dep_samba_dcerpc_base == NULL)
		return;

	MInterfacePointer_Type = (PyTypeObject *)PyObject_GetAttrString(dep_samba_dcerpc_orpc, "MInterfacePointer");
	if (MInterfacePointer_Type == NULL)
		return;

	ORPCTHAT_Type = (PyTypeObject *)PyObject_GetAttrString(dep_samba_dcerpc_orpc, "ORPCTHAT");
	if (ORPCTHAT_Type == NULL)
		return;

	ORPCTHIS_Type = (PyTypeObject *)PyObject_GetAttrString(dep_samba_dcerpc_orpc, "ORPCTHIS");
	if (ORPCTHIS_Type == NULL)
		return;

	COMVERSION_Type = (PyTypeObject *)PyObject_GetAttrString(dep_samba_dcerpc_orpc, "COMVERSION");
	if (COMVERSION_Type == NULL)
		return;

	DUALSTRINGARRAY_Type = (PyTypeObject *)PyObject_GetAttrString(dep_samba_dcerpc_orpc, "DUALSTRINGARRAY");
	if (DUALSTRINGARRAY_Type == NULL)
		return;

	GUID_Type = (PyTypeObject *)PyObject_GetAttrString(dep_samba_dcerpc_misc, "GUID");
	if (GUID_Type == NULL)
		return;

	ClientConnection_Type = (PyTypeObject *)PyObject_GetAttrString(dep_samba_dcerpc_base, "ClientConnection");
	if (ClientConnection_Type == NULL)
		return;

	IRemoteActivation_InterfaceType.tp_base = ClientConnection_Type;

	if (PyType_Ready(&IRemoteActivation_InterfaceType) < 0)
		return;
	if (!PyInterface_AddNdrRpcMethods(&IRemoteActivation_InterfaceType, py_ndr_IRemoteActivation_methods))
		return;

#ifdef PY_IREMOTEACTIVATION_PATCH
	PY_IREMOTEACTIVATION_PATCH(&IRemoteActivation_InterfaceType);
#endif

	m = Py_InitModule3("remact", remact_methods, "remact DCE/RPC");
	if (m == NULL)
		return;

	PyModule_AddObject(m, "MODE_GET_CLASS_OBJECT", PyInt_FromLong(0xffffffff));
	PyModule_AddObject(m, "RPC_C_IMP_LEVEL_DELEGATE", PyInt_FromLong(RPC_C_IMP_LEVEL_DELEGATE));
	PyModule_AddObject(m, "RPC_C_IMP_LEVEL_IDENTIFY", PyInt_FromLong(RPC_C_IMP_LEVEL_IDENTIFY));
	PyModule_AddObject(m, "RPC_C_IMP_LEVEL_ANONYMOUS", PyInt_FromLong(RPC_C_IMP_LEVEL_ANONYMOUS));
	PyModule_AddObject(m, "RPC_C_IMP_LEVEL_IMPERSONATE", PyInt_FromLong(RPC_C_IMP_LEVEL_IMPERSONATE));
	PyModule_AddObject(m, "RPC_C_IMP_LEVEL_DEFAULT", PyInt_FromLong(RPC_C_IMP_LEVEL_DEFAULT));
	Py_INCREF((PyObject *)(void *)&IRemoteActivation_InterfaceType);
	PyModule_AddObject(m, "IRemoteActivation", (PyObject *)(void *)&IRemoteActivation_InterfaceType);
#ifdef PY_MOD_REMACT_PATCH
	PY_MOD_REMACT_PATCH(m);
#endif

}
