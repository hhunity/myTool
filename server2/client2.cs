
// Single-file sample: multi-endpoint + DI + share across screens (WPF想定)
// - 接続ごとに EndpointStore + ApiClient を持つ（camA/camB...）
// - 画面間共有は ConnectionManager(=Singleton) から key で取り出す
// - BaseAddress は動的に変えず、毎回 new Uri(base, path) で絶対URIを作る

using System;
using System.Collections.Concurrent;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;

namespace MultiEndpointSample
{
    // ----------------------------
    // 1) 変更されうる「接続先」を保持する（1接続につき1個）
    // ----------------------------
    public sealed class EndpointStore
    {
        private readonly object _gate = new();
        private Uri _baseUri = new Uri("http://localhost:8080/");

        public Uri BaseUri
        {
            get { lock (_gate) return _baseUri; }
        }

        public bool TrySetBaseUrl(string? text)
        {
            if (string.IsNullOrWhiteSpace(text)) return false;
            if (!Uri.TryCreate(text.Trim(), UriKind.Absolute, out var uri)) return false;
            if (uri.Scheme != Uri.UriSchemeHttp && uri.Scheme != Uri.UriSchemeHttps) return false;

            // 末尾 / を揃える（事故防止）
            var s = uri.ToString();
            if (!s.EndsWith("/")) s += "/";

            lock (_gate) _baseUri = new Uri(s, UriKind.Absolute);
            return true;
        }

        public Uri Combine(string relativePath) => new Uri(BaseUri, relativePath);
    }

    // ----------------------------
    // 2) 実際のAPI呼び出し（EndpointStore を見て毎回URIを作る）
    // ----------------------------
    public sealed class ApiClient
    {
        private readonly HttpClient _http;
        private readonly EndpointStore _ep;

        public ApiClient(HttpClient http, EndpointStore ep)
        {
            _http = http;
            _ep = ep;
        }

        public Task<HttpResponseMessage> StartCaptureAsync(CancellationToken ct = default)
        {
            var uri = _ep.Combine("api/capture/start");
            return _http.PostAsync(uri, content: null, ct);
        }

        public Task<HttpResponseMessage> StopCaptureAsync(CancellationToken ct = default)
        {
            var uri = _ep.Combine("api/capture/stop");
            return _http.PostAsync(uri, content: null, ct);
        }

        public Task<HttpResponseMessage> GetStateAsync(CancellationToken ct = default)
        {
            var uri = _ep.Combine("api/state");
            return _http.GetAsync(uri, ct);
        }
    }

    // ----------------------------
    // 3) 1接続ぶんの束（他画面でも同じ key で取得して使う）
    // ----------------------------
    public sealed class ConnectionSession
    {
        public string Key { get; }
        public EndpointStore Endpoint { get; }
        public ApiClient Api { get; }

        public ConnectionSession(string key, EndpointStore endpoint, ApiClient api)
        {
            Key = key;
            Endpoint = endpoint;
            Api = api;
        }
    }

    // ----------------------------
    // 4) 接続を保持するレジストリ（Singleton）
    //    画面間共有はここから key で取り出す
    // ----------------------------
    public sealed class ConnectionManager
    {
        private readonly IHttpClientFactory _httpFactory;
        private readonly ConcurrentDictionary<string, ConnectionSession> _sessions = new();

        public ConnectionManager(IHttpClientFactory httpFactory)
        {
            _httpFactory = httpFactory;
        }

        // key が同じなら同じ接続を返す（無ければ作る）
        public ConnectionSession GetOrCreate(string key, string baseUrl)
        {
            return _sessions.GetOrAdd(key, _ =>
            {
                var ep = new EndpointStore();
                if (!ep.TrySetBaseUrl(baseUrl))
                    throw new ArgumentException($"Invalid baseUrl: {baseUrl}", nameof(baseUrl));

                var http = _httpFactory.CreateClient(); // 共通設定は AddHttpClient 側で
                var api = new ApiClient(http, ep);

                return new ConnectionSession(key, ep, api);
            });
        }

        public bool TryGet(string key, out ConnectionSession? session)
            => _sessions.TryGetValue(key, out session);

        public ConnectionSession GetRequired(string key)
            => _sessions.TryGetValue(key, out var s)
                ? s
                : throw new InvalidOperationException($"No session for key='{key}'");

        public bool Remove(string key) => _sessions.TryRemove(key, out _);
    }

    // ----------------------------
    // 5) 「今どれを操作対象にしてるか」を共有したいとき用（Singleton）
    // ----------------------------
    public sealed class AppState
    {
        public string? CurrentConnectionKey { get; set; } // "camA" とか
    }

    // ----------------------------
    // 6) 画面(ViewModel)側の利用例
    //    ※ 他画面でも同様に ConnectionManager を注入して key で取る
    // ----------------------------
    public sealed class MainViewModel
    {
        private readonly ConnectionManager _cm;
        private readonly AppState _state;

        public MainViewModel(ConnectionManager cm, AppState state)
        {
            _cm = cm;
            _state = state;
        }

        public void EnsureConnections()
        {
            // 例: 2台登録（設定画面から呼ぶ想定でもOK）
            _cm.GetOrCreate("camA", "http://192.168.0.10:8080/");
            _cm.GetOrCreate("camB", "http://192.168.0.11:8080/");
            _state.CurrentConnectionKey ??= "camA";
        }

        public Task StartOnCurrentAsync(CancellationToken ct = default)
        {
            var key = _state.CurrentConnectionKey
                      ?? throw new InvalidOperationException("No selected connection");
            var s = _cm.GetRequired(key);
            return s.Api.StartCaptureAsync(ct);
        }
    }

    public sealed class SettingsViewModel
    {
        private readonly ConnectionManager _cm;
        private readonly AppState _state;

        public SettingsViewModel(ConnectionManager cm, AppState state)
        {
            _cm = cm;
            _state = state;
        }

        public void SelectCurrent(string key)
        {
            _state.CurrentConnectionKey = key;
        }

        // 既存接続のURLを変えたい場合（EndpointStore を直接更新）
        // ※ ここでは簡易に GetRequired して更新している
        public bool TryUpdateBaseUrl(string key, string baseUrl)
        {
            var s = _cm.GetRequired(key);
            return s.Endpoint.TrySetBaseUrl(baseUrl);
        }
    }

    // ----------------------------
    // 7) DIセットアップ例（WPFなら App.xaml.cs で同様に Host を持つ）
    // ----------------------------
    public static class DiSetup
    {
        public static IHost BuildHost()
        {
            return Host.CreateDefaultBuilder()
                .ConfigureServices(services =>
                {
                    services.AddHttpClient();               // IHttpClientFactory
                    services.AddSingleton<ConnectionManager>();
                    services.AddSingleton<AppState>();

                    // ViewModels（WindowをDI生成するならWindowもAddTransient）
                    services.AddTransient<MainViewModel>();
                    services.AddTransient<SettingsViewModel>();
                })
                .Build();
        }
    }
}


public sealed class ConnectionSession
{
    public string Key { get; }
    public EndpointStore Endpoint { get; }
    public ApiClient Api { get; }

    public ConnectionSession(string key, EndpointStore endpoint, ApiClient api)
    {
        Key = key;
        Endpoint = endpoint;
        Api = api;
    }
}

using System;
using System.Collections.Concurrent;
using System.Net.Http;

public sealed class ConnectionFactory
{
    private readonly IHttpClientFactory _httpFactory;

    // 必要なら再利用（同じKeyなら同じSessionを返す）
    private readonly ConcurrentDictionary<string, ConnectionSession> _sessions = new();

    public ConnectionFactory(IHttpClientFactory httpFactory)
    {
        _httpFactory = httpFactory;
    }

    public ConnectionSession GetOrCreate(string key, string baseUrl)
    {
        return _sessions.GetOrAdd(key, _ =>
        {
            var ep = new EndpointStore();              // 1接続に1個
            if (!ep.TrySetBaseUrl(baseUrl)) throw new ArgumentException("Invalid baseUrl");

            var http = _httpFactory.CreateClient();    // HttpClientはFactoryから
            var api  = new ApiClient(http, ep);        // epを注入して束ねる

            return new ConnectionSession(key, ep, api);
        });
    }

    public bool TryRemove(string key) => _sessions.TryRemove(key, out _);
}

// 例：ユーザーが2つ登録した
var s1 = _connFactory.GetOrCreate("camA", "http://192.168.0.10:8080/");
var s2 = _connFactory.GetOrCreate("camB", "http://192.168.0.11:8080/");

await s1.Api.StartAsync();
await s2.Api.StartAsync();






using System;
using System.Threading;

public sealed class EndpointStore
{
    // 変更通知（必要ないなら消してOK）
    public event EventHandler<Uri>? BaseUriChanged;

    // “現在値”をロックなしで読むために volatile + Interlocked を使う
    private Uri _baseUri;

    public EndpointStore(string defaultBaseUrl = "http://localhost:8080/")
    {
        if (!Uri.TryCreate(defaultBaseUrl, UriKind.Absolute, out var uri))
            uri = new Uri("http://localhost:8080/");

        _baseUri = NormalizeBaseUri(uri);
    }

    /// <summary>
    /// 現在のBaseUri（常に末尾スラッシュ付き）
    /// </summary>
    public Uri BaseUri => Volatile.Read(ref _baseUri);

    /// <summary>
    /// GUI入力文字列を検証して BaseUri を更新する（成功時 true）
    /// </summary>
    public bool TrySetBaseUrl(string? text, out string? error)
    {
        if (string.IsNullOrWhiteSpace(text))
        {
            error = "URL が空です。例: http://localhost:8080/";
            return false;
        }

        if (!Uri.TryCreate(text.Trim(), UriKind.Absolute, out var uri))
        {
            error = "URLの形式が不正です。例: http://localhost:8080/";
            return false;
        }

        // 必要なら http/https のみに制限
        if (uri.Scheme != Uri.UriSchemeHttp && uri.Scheme != Uri.UriSchemeHttps)
        {
            error = "http または https の URL を入力してください。";
            return false;
        }

        // 任意: ホスト必須（http://:8080/ みたいなのを弾く）
        if (string.IsNullOrWhiteSpace(uri.Host))
        {
            error = "ホスト名がありません。例: http://localhost:8080/";
            return false;
        }

        var normalized = NormalizeBaseUri(uri);

        // 変化がないなら通知しない
        var prev = Interlocked.Exchange(ref _baseUri, normalized);
        if (!UriEquals(prev, normalized))
        {
            BaseUriChanged?.Invoke(this, normalized);
        }

        error = null;
        return true;
    }

    /// <summary>
    /// 例外を投げる版（内部用）
    /// </summary>
    public void SetBaseUri(Uri uri)
    {
        if (uri is null) throw new ArgumentNullException(nameof(uri));
        if (!uri.IsAbsoluteUri) throw new ArgumentException("Absolute URI が必要です。", nameof(uri));
        if (uri.Scheme != Uri.UriSchemeHttp && uri.Scheme != Uri.UriSchemeHttps)
            throw new ArgumentException("http/https のみ対応です。", nameof(uri));

        var normalized = NormalizeBaseUri(uri);
        var prev = Interlocked.Exchange(ref _baseUri, normalized);
        if (!UriEquals(prev, normalized))
        {
            BaseUriChanged?.Invoke(this, normalized);
        }
    }

    /// <summary>
    /// ベースURIと相対パスから絶対URIを作る（スラッシュ事故を回避）
    /// </summary>
    public Uri Combine(string relativePath)
    {
        if (relativePath is null) throw new ArgumentNullException(nameof(relativePath));
        return new Uri(BaseUri, relativePath);
    }

    private static Uri NormalizeBaseUri(Uri uri)
    {
        // fragment/query はベースとしては不要なことが多いので落とす（必要なら消してOK）
        var b = new UriBuilder(uri) { Fragment = "", Query = "" };

        // 末尾スラッシュを保証（"http://x:8080" → "http://x:8080/")
        var s = b.Uri.ToString();
        if (!s.EndsWith("/")) s += "/";

        return new Uri(s, UriKind.Absolute);
    }

    private static bool UriEquals(Uri a, Uri b)
        => string.Equals(a.ToString(), b.ToString(), StringComparison.OrdinalIgnoreCase);
}

using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

public sealed class DeviceStateDto
{
    public string State { get; set; } = "Unknown";
    public string? Message { get; set; }
}

public sealed class StateChangedEventArgs : EventArgs
{
    public string State { get; }
    public string RawJson { get; }
    public long? EventId { get; }

    public StateChangedEventArgs(string state, string rawJson, long? eventId)
    {
        State = state;
        RawJson = rawJson;
        EventId = eventId;
    }
}

/// <summary>
/// SSE(stateイベントのみ)を購読しつつ、GET /state で再同期できる状態管理。
/// WaitForStateAsync で「期待状態になるまで待つ」を実現する。
/// </summary>
public sealed class DeviceStateMonitor : IDisposable
{
    private readonly HttpClient _http;
    private readonly Uri _baseUri;
    private readonly Uri _eventsUri;
    private readonly JsonSerializerOptions _jsonOpt = new() { PropertyNameCaseInsensitive = true };

    private readonly object _gate = new();
    private readonly CancellationTokenSource _cts = new();
    private Task? _runner;

    private string _currentState = "Unknown";
    private long? _lastEventId;

    // 状態変化通知（WPFならDispatcherでUIスレッドに戻して使う）
    public event EventHandler<StateChangedEventArgs>? StateChanged;
    public event Action<Exception>? Error;

    public string CurrentState
    {
        get { lock (_gate) return _currentState; }
        private set { lock (_gate) _currentState = value; }
    }

    public long? LastEventId
    {
        get { lock (_gate) return _lastEventId; }
        private set { lock (_gate) _lastEventId = value; }
    }

    public DeviceStateMonitor(HttpClient http, Uri baseUri)
    {
        _http = http;
        _baseUri = baseUri;
        _eventsUri = new Uri(baseUri, "/events");
    }

    public void Start()
    {
        if (_runner != null) return;
        _runner = RunAsync(_cts.Token);
    }

    public async Task StopAsync()
    {
        _cts.Cancel();
        if (_runner != null)
        {
            try { await _runner.ConfigureAwait(false); }
            catch { /* ignore */ }
        }
    }

    public void Dispose()
    {
        _cts.Cancel();
        _cts.Dispose();
    }

    /// <summary>
    /// GET /state で現在状態を取得して CurrentState を更新する
    /// </summary>
    public async Task ResyncStateAsync(CancellationToken ct)
    {
        var uri = new Uri(_baseUri, "/state");
        using var resp = await _http.GetAsync(uri, ct).ConfigureAwait(false);
        resp.EnsureSuccessStatusCode();

        var json = await resp.Content.ReadAsStringAsync(ct).ConfigureAwait(false);
        var dto = JsonSerializer.Deserialize<DeviceStateDto>(json, _jsonOpt) ?? new DeviceStateDto();

        UpdateState(dto.State, json, eventId: null);
    }

    /// <summary>
    /// 期待する状態になるまで待つ。タイムアウト時はGET /stateで再同期してから判定する。
    /// </summary>
    public async Task<bool> WaitForStateAsync(
        Func<string, bool> predicate,
        TimeSpan timeout,
        TimeSpan? resyncInterval = null,
        CancellationToken ct = default)
    {
        // まず現在状態で即判定
        if (predicate(CurrentState)) return true;

        using var timeoutCts = CancellationTokenSource.CreateLinkedTokenSource(ct);
        timeoutCts.CancelAfter(timeout);

        var tcs = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);

        void Handler(object? _, StateChangedEventArgs e)
        {
            if (predicate(e.State))
                tcs.TrySetResult(true);
        }

        StateChanged += Handler;

        try
        {
            // 定期再同期（SSE欠落/切断/欠番の実運用対策）
            var interval = resyncInterval ?? TimeSpan.FromSeconds(2);
            var nextResync = DateTime.UtcNow + interval;

            while (!timeoutCts.IsCancellationRequested)
            {
                // SSEで状態が来たら即解決する
                var completed = await Task.WhenAny(tcs.Task, Task.Delay(200, timeoutCts.Token)).ConfigureAwait(false);
                if (completed == tcs.Task) return true;

                // 一定間隔でGET /stateを叩いて再同期（SSEが切れてても復旧）
                if (DateTime.UtcNow >= nextResync)
                {
                    try
                    {
                        await ResyncStateAsync(timeoutCts.Token).ConfigureAwait(false);
                        if (predicate(CurrentState)) return true;
                    }
                    catch (Exception ex)
                    {
                        Error?.Invoke(ex);
                    }
                    nextResync = DateTime.UtcNow + interval;
                }
            }

            // タイムアウト直前に最後の再同期を一回やる（最後の保険）
            try
            {
                await ResyncStateAsync(CancellationToken.None).ConfigureAwait(false);
                if (predicate(CurrentState)) return true;
            }
            catch { /* ignore */ }

            return false;
        }
        finally
        {
            StateChanged -= Handler;
        }
    }

    /// <summary>
    /// 便利メソッド：特定状態名を待つ
    /// </summary>
    public Task<bool> WaitForStateAsync(
        string expectedState,
        TimeSpan timeout,
        TimeSpan? resyncInterval = null,
        CancellationToken ct = default)
        => WaitForStateAsync(s => string.Equals(s, expectedState, StringComparison.OrdinalIgnoreCase),
                             timeout, resyncInterval, ct);

    private async Task RunAsync(CancellationToken ct)
    {
        // 起動時同期
        try { await ResyncStateAsync(ct).ConfigureAwait(false); }
        catch (Exception ex) { Error?.Invoke(ex); }

        // 再接続ループ
        var backoffMs = 200;
        const int backoffMax = 5000;

        while (!ct.IsCancellationRequested)
        {
            try
            {
                await ConsumeSseOnceAsync(ct).ConfigureAwait(false);
                backoffMs = 200;
            }
            catch (OperationCanceledException) when (ct.IsCancellationRequested)
            {
                break;
            }
            catch (Exception ex)
            {
                Error?.Invoke(ex);

                // 切断時は状態再同期してから再接続
                try { await ResyncStateAsync(ct).ConfigureAwait(false); }
                catch (Exception rex) { Error?.Invoke(rex); }

                await Task.Delay(backoffMs, ct).ConfigureAwait(false);
                backoffMs = Math.Min(backoffMs * 2, backoffMax);
            }
        }
    }

    private async Task ConsumeSseOnceAsync(CancellationToken ct)
    {
        using var req = new HttpRequestMessage(HttpMethod.Get, _eventsUri);
        req.Headers.Accept.Clear();
        req.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("text/event-stream"));
        req.Headers.CacheControl = new CacheControlHeaderValue { NoCache = true };

        // サーバがLast-Event-IDに対応していれば、取りこぼし再送が可能
        if (LastEventId is long lastId)
            req.Headers.TryAddWithoutValidation("Last-Event-ID", lastId.ToString());

        using var resp = await _http.SendAsync(req, HttpCompletionOption.ResponseHeadersRead, ct).ConfigureAwait(false);
        resp.EnsureSuccessStatusCode();

        await using var stream = await resp.Content.ReadAsStreamAsync(ct).ConfigureAwait(false);
        using var reader = new StreamReader(stream, Encoding.UTF8);

        long? currentId = null;
        string? currentEvent = null;
        var dataLines = new List<string>();

        while (!ct.IsCancellationRequested)
        {
            var line = await reader.ReadLineAsync().ConfigureAwait(false);
            if (line == null) throw new IOException("SSE stream ended.");

            if (line.Length == 0)
            {
                // イベント終端
                if (dataLines.Count > 0 || currentId.HasValue || currentEvent != null)
                {
                    var evName = currentEvent ?? "";
                    var data = string.Join("\n", dataLines);

                    // stateイベントだけ処理（あなたの前提）
                    if (string.Equals(evName, "state", StringComparison.OrdinalIgnoreCase) || string.IsNullOrEmpty(evName))
                    {
                        var state = TryParseStateFromJson(data) ?? "Unknown";
                        if (currentId.HasValue) LastEventId = currentId.Value;

                        UpdateState(state, data, currentId);
                    }
                }

                currentId = null;
                currentEvent = null;
                dataLines.Clear();
                continue;
            }

            if (line.StartsWith(":", StringComparison.Ordinal)) continue; // comment

            var idx = line.IndexOf(':');
            var field = idx >= 0 ? line[..idx] : line;
            var value = idx >= 0 ? line[(idx + 1)..] : "";
            if (value.StartsWith(" ", StringComparison.Ordinal)) value = value[1..];

            switch (field)
            {
                case "id":
                    if (long.TryParse(value, out var id)) currentId = id;
                    break;
                case "event":
                    currentEvent = value;
                    break;
                case "data":
                    dataLines.Add(value);
                    break;
            }
        }
    }

    private void UpdateState(string newState, string rawJson, long? eventId)
    {
        var old = CurrentState;
        CurrentState = newState;

        // 状態変化がなくても通知してよい（UI更新/ログ用途）。
        // 変化時のみ通知にしたいなら old != newState でガードする。
        StateChanged?.Invoke(this, new StateChangedEventArgs(newState, rawJson, eventId));
    }

    private string? TryParseStateFromJson(string json)
    {
        try
        {
            // {"state":"Connected"} の想定
            using var doc = JsonDocument.Parse(json);
            if (doc.RootElement.TryGetProperty("state", out var st) && st.ValueKind == JsonValueKind.String)
                return st.GetString();
        }
        catch { /* ignore */ }
        return null;
    }
}

public sealed class DeviceController
{
    private readonly HttpClient _http;
    private readonly Uri _baseUri;
    private readonly DeviceStateMonitor _mon;

    public DeviceController(HttpClient http, Uri baseUri, DeviceStateMonitor mon)
    {
        _http = http;
        _baseUri = baseUri;
        _mon = mon;
    }

    public async Task<bool> ConnectAsync(TimeSpan timeout, CancellationToken ct)
    {
        using var resp = await _http.PostAsync(new Uri(_baseUri, "/connect"), content: null, ct).ConfigureAwait(false);
        resp.EnsureSuccessStatusCode();

        // 期待状態：Connected（途中はConnectingが来てもOK）
        return await _mon.WaitForStateAsync("Connected", timeout, resyncInterval: TimeSpan.FromSeconds(2), ct).ConfigureAwait(false);
    }

    public async Task<bool> StartCaptureAsync(TimeSpan timeout, CancellationToken ct)
    {
        using var resp = await _http.PostAsync(new Uri(_baseUri, "/capture/start"), content: null, ct).ConfigureAwait(false);
        resp.EnsureSuccessStatusCode();

        return await _mon.WaitForStateAsync("Capturing", timeout, TimeSpan.FromSeconds(2), ct).ConfigureAwait(false);
    }

    public async Task<bool> StopCaptureAsync(TimeSpan timeout, CancellationToken ct)
    {
        using var resp = await _http.PostAsync(new Uri(_baseUri, "/capture/stop"), content: null, ct).ConfigureAwait(false);
        resp.EnsureSuccessStatusCode();

        // 停止完了＝Connected に戻る想定（あなたの定義に合わせて変更）
        return await _mon.WaitForStateAsync("Connected", timeout, TimeSpan.FromSeconds(2), ct).ConfigureAwait(false);
    }
}

using System;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;

public static class Example
{
    public static async Task Main()
    {
        var baseUri = new Uri("http://localhost:8080");
        using var http = new HttpClient();

        using var mon = new DeviceStateMonitor(http, baseUri);
        mon.Error += ex => Console.WriteLine($"[ERR] {ex.Message}");
        mon.StateChanged += (_, e) =>
        {
            Console.WriteLine($"[STATE] {e.State} (id={e.EventId}) raw={e.RawJson}");
            // WPFならここでDispatcher経由でVM更新
        };

        mon.Start();

        var ctrl = new DeviceController(http, baseUri, mon);

        using var cts = new CancellationTokenSource();

        var ok1 = await ctrl.ConnectAsync(TimeSpan.FromSeconds(5), cts.Token);
        Console.WriteLine($"Connect => {ok1}");

        var ok2 = await ctrl.StartCaptureAsync(TimeSpan.FromSeconds(5), cts.Token);
        Console.WriteLine($"Start => {ok2}");

        var ok3 = await ctrl.StopCaptureAsync(TimeSpan.FromSeconds(5), cts.Token);
        Console.WriteLine($"Stop => {ok3}");

        await mon.StopAsync();
    }
}

