#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "agent_runtime.h"
#include "mcp_gateway.h"
#include "mcp_server.h"

static char *dummy_tool_cb(const char *fn_name, size_t argc, const char **argv, int *out_is_error) {
    if (out_is_error) *out_is_error = 0;
    char *buf = malloc(128);
    snprintf(buf, 128, "%s:%zu:%s", fn_name, argc, (argc > 0 && argv[0]) ? argv[0] : "");
    return buf;
}

int main(void) {
    agent_runtime_reset();

    /* 1. Register controlled stdio MCP test server */
    McpStatus st = mcp_gateway_register_server(
        "mcp_test",
        MCP_TRANSPORT_STDIO,
        "python3 tests/mock_mcp_server.py",
        "",
        2000,
        4
    );
    assert(st == MCP_STATUS_OK);

    /* 2. Connect & negotiate MCP protocol + discover tools & resources */
    size_t tool_count = 0;
    st = mcp_gateway_discover_tools("mcp_test", &tool_count);
    assert(st == MCP_STATUS_OK);
    assert(tool_count == 6);

    size_t res_count = 0;
    st = mcp_gateway_discover_resources("mcp_test", &res_count);
    assert(st == MCP_STATUS_OK);
    assert(res_count == 2);

    const McpServerEntry *srv = mcp_gateway_get_server("mcp_test");
    assert(srv != NULL);
    assert(srv->connected == 1);
    assert(strcmp(srv->negotiated_version, MCP_PROTOCOL_VERSION) == 0);
    assert(srv->capabilities_tools == 1);
    assert(srv->capabilities_resources == 1);

    McpCallResult rres = mcp_gateway_read_resource("mcp_test", "resource://pifari/runtime_spec", 1000);
    assert(rres.status == MCP_STATUS_OK);
    assert(strstr(rres.output_text, "Pifari-Agent-Runtime") != NULL);

    /* 3. Register 3 agents with distinct roles */
    assert(agent_runtime_register_agent("ag_collector", "Data Collector", "Coletar series", "local-deterministic", 2, 1500) == 0);
    assert(agent_runtime_register_agent("ag_analyst", "Statistical Analyst", "Analisar series", "local-deterministic", 2, 1500) == 0);
    assert(agent_runtime_register_agent("ag_reporter", "Executive Reporter", "Gerar relatorio", "local-deterministic", 1, 1500) == 0);

    const ArAgentEntry *ag1 = agent_runtime_get_agent("ag_collector");
    assert(ag1 != NULL && ag1->version == 1);

    /* 4. Policy Engine: default-deny blocks unauthorized tools and side effects */
    McpCallResult denied = agent_runtime_invoke_mcp("ag_collector", "mcp_test", "restricted_op", "{}", 1000);
    assert(denied.status == MCP_STATUS_ERR_POLICY_DENIED);

    /* Authorize read-only tools for the 3 agents */
    assert(agent_runtime_policy_allow("ag_collector", "mcp_test", "collect_dataset", 0) == 0);
    assert(agent_runtime_policy_allow("ag_collector", "mcp_test", "slow_op", 0) == 0);
    assert(agent_runtime_policy_allow("ag_analyst", "mcp_test", "analyze_series", 0) == 0);
    assert(agent_runtime_policy_allow("ag_reporter", "mcp_test", "build_report", 0) == 0);

    /* Side-effect tool without allow_side_effects=1 must still be denied */
    assert(agent_runtime_policy_allow("ag_reporter", "mcp_test", "write_audit_entry", 0) == 0);
    McpCallResult se_denied = agent_runtime_invoke_mcp(
        "ag_reporter", "mcp_test", "write_audit_entry", "{\"entry\":\"test1\"}", 1000
    );
    assert(se_denied.status == MCP_STATUS_ERR_POLICY_DENIED);

    /* Grant side-effect permission and verify duplicate side-effect protection */
    assert(agent_runtime_policy_allow("ag_reporter", "mcp_test", "write_audit_entry", 1) == 0);
    McpCallResult se_ok = agent_runtime_invoke_mcp(
        "ag_reporter", "mcp_test", "write_audit_entry", "{\"entry\":\"commit_01\"}", 1000
    );
    assert(se_ok.status == MCP_STATUS_OK);
    assert(strstr(se_ok.output_text, "AUDIT_COMMITTED:commit_01") != NULL);

    McpCallResult se_dup = agent_runtime_invoke_mcp(
        "ag_reporter", "mcp_test", "write_audit_entry", "{\"entry\":\"commit_01\"}", 1000
    );
    assert(se_dup.status == MCP_STATUS_ERR_DUPLICATE_CALL);

    /* 5. Context Store versioning and no-op invariance */
    int v1 = agent_runtime_ctx_put("config.mode", "production_test", "ag_collector", "init");
    assert(v1 == 1);
    int v_noop = agent_runtime_ctx_put("config.mode", "production_test", "ag_collector", "init");
    assert(v_noop == 1);
    int v2 = agent_runtime_ctx_put("config.mode", "verified_mode", "ag_collector", "init");
    assert(v2 == 2);
    assert(strcmp(agent_runtime_ctx_get("config.mode"), "verified_mode") == 0);

    /* 6. Task Engine & Parallel DAG Execution with Dependencies and Cancellation */
    assert(agent_runtime_create_task(
        "task_collect", "ag_collector", "mcp_test", "collect_dataset",
        "{\"source\":\"sensor_alpha\"}", "ctx.raw_dataset", 10, 1500
    ) == 0);

    assert(agent_runtime_create_task(
        "task_analyze", "ag_analyst", "mcp_test", "analyze_series",
        "{\"dataset\":\"sensor_alpha_baseline\"}", "ctx.analysis", 10, 1500
    ) == 0);

    /* Dependent task waiting on both task_collect and task_analyze */
    assert(agent_runtime_create_task(
        "task_report", "ag_reporter", "mcp_test", "build_report",
        "{\"collector_summary\":\"{{ctx:ctx.raw_dataset}}\",\"analysis_summary\":\"{{ctx:ctx.analysis}}\"}",
        "ctx.final_report", 5, 1500
    ) == 0);
    assert(agent_runtime_task_add_dep("task_report", "task_collect") == 0);
    assert(agent_runtime_task_add_dep("task_report", "task_analyze") == 0);

    /* Task that exceeds deadline and gets cancelled by timeout */
    assert(agent_runtime_create_task(
        "task_slow_timeout", "ag_collector", "mcp_test", "slow_op",
        "{\"delay_ms\":350}", "", 1, 70
    ) == 0);

    /* Task explicitly cancelled before execution */
    assert(agent_runtime_create_task(
        "task_explicit_cancel", "ag_collector", "mcp_test", "collect_dataset",
        "{\"source\":\"aborted\"}", "", 1, 1000
    ) == 0);
    assert(agent_runtime_task_cancel("task_explicit_cancel", "cancelado_pelo_coordenador") == 0);

    int completed = agent_runtime_dag_run(2);
    assert(completed == 3);

    const ArTaskEntry *tc = agent_runtime_get_task("task_collect");
    const ArTaskEntry *ta = agent_runtime_get_task("task_analyze");
    const ArTaskEntry *tr = agent_runtime_get_task("task_report");
    const ArTaskEntry *ts = agent_runtime_get_task("task_slow_timeout");
    const ArTaskEntry *tx = agent_runtime_get_task("task_explicit_cancel");

    assert(tc && tc->state == TASK_STATE_COMPLETED);
    assert(ta && ta->state == TASK_STATE_COMPLETED);
    assert(tr && tr->state == TASK_STATE_COMPLETED);
    assert(ts && ts->state == TASK_STATE_CANCELLED);
    assert(tx && tx->state == TASK_STATE_CANCELLED);
    assert(strstr(tr->result_text, "RELATORIO_CONSOLIDADO") != NULL);
    assert(strstr(tr->result_text, "sensor_alpha") != NULL);

    /* 7. Artifact Manager & Telemetry Engine */
    assert(agent_runtime_artifact_save(
        "art_unit_report",
        "/tmp/pifari_test_artifact.txt",
        tr->result_text,
        "ag_reporter",
        "task_report"
    ) == 0);

    const ArArtifactEntry *art = agent_runtime_get_artifact("art_unit_report");
    assert(art != NULL);
    assert(art->byte_size == strlen(tr->result_text));
    assert(strlen(art->checksum_fnv1a) == 16);

    assert(agent_runtime_progress_percent() == 100);
    assert(agent_runtime_telemetry_count() >= 10);
    assert(agent_runtime_export_report_json("/tmp/pifari_test_telemetry.json") == 0);

    unlink("/tmp/pifari_test_artifact.txt");
    unlink("/tmp/pifari_test_telemetry.json");
    agent_runtime_reset();

    /* 8. Native MCP Server Engine C unit verification */
    pifari_mcp_server_init("unit-srv", "1.0.0");
    const char *params[1] = {"x"};
    assert(pifari_mcp_server_register_tool("dobro", "Dobra valor", "fn_dobro", 1, params, 1) == 0);
    assert(pifari_mcp_server_register_resource("resource://unit/info", "info", "application/json", "{\"ok\":true}") == 0);
    char rpc_out[2048];
    assert(pifari_mcp_server_handle_line("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}", dummy_tool_cb, rpc_out, sizeof(rpc_out)) == 1);
    assert(strstr(rpc_out, "2025-03-26") != NULL);
    assert(pifari_mcp_server_handle_line("{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"dobro\",\"arguments\":{\"x\":\"42\"}}}", dummy_tool_cb, rpc_out, sizeof(rpc_out)) == 1);
    assert(strstr(rpc_out, "fn_dobro:1:42") != NULL);

    printf("test_agent_runtime: ok\n");
    return 0;
}
