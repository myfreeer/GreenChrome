function AJAX(url, data, callback)
{
	$.ajax({
		url: url,
		type: "post",
		async: true,
		data: data,
		error: function(e) {
			alert("发生未知错误");
		},
		dataType: "json",
		success: function(response) {
			callback(response)
		}
	});
}

function Render(response)
{
	// 基本设置
	$("#UserData").val(response["基本设置"]["数据目录"])
	$("#BossKey").val(response["基本设置"]["老板键"])
	$("#HTMLFile").val(response["基本设置"]["新标签页面"])
	$('#RemoveUpdateError').attr("checked",response["基本设置"]["移除更新错误"]=="1");
	$('#KillAtEnd').attr("checked",response["基本设置"]["自动结束运行程序"]=="1");
	$('#RecoveryNPAPI').attr("checked",response["基本设置"]["恢复NPAPI"]=="1");
	$('#MakePortable').attr("checked",response["基本设置"]["便携化"]=="1");

	// 界面增强
	$('#DoubleClickCloseTab').attr("checked",response["界面增强"]["双击关闭标签页"]=="1");
	$('#RightClickCloseTab').attr("checked",response["界面增强"]["右键关闭标签页"]=="1");
	$('#KeepLastTab').attr("checked",response["界面增强"]["保留最后标签"]=="1");
	$('#HoverTabSwitch').attr("checked",response["界面增强"]["悬停快速标签切换"]=="1");
	$('#RightTabSwitch').attr("checked",response["界面增强"]["右键快速标签切换"]=="1");
	$('#BookMarkNewTab').attr("checked",response["界面增强"]["新标签打开书签"]=="1");
	$('#OpenUrlNewTab').attr("checked",response["界面增强"]["新标签打开网址"]=="1");
	$('#NotBlankTab').attr("checked",response["界面增强"]["新标签页不生效"]=="1");
	$('#FrontNewTab').attr("checked",response["界面增强"]["前台打开新标签"]=="1");

	// 追加参数
	var remove_section = '<a href="#" class="remove_section"><span class="glyphicon glyphicon-remove pull-right"></span></a>'
	$("#additional_parameter").empty()
	for(var i = 0 ;i < response["追加参数"].length; i++)
	{
		var parameter = response["追加参数"][i];
		$("#additional_parameter").append('<li class="list-group-item" data-section="追加参数">' + parameter + remove_section + '</li>');
	}

	// 启动时运行
	$("#start_program").empty()
	for(var i = 0 ;i < response["启动时运行"].length; i++)
	{
		var parameter = response["启动时运行"][i];
		$("#start_program").append('<li class="list-group-item" data-section="启动时运行">' + parameter + remove_section + '</li>');
	}

	// 关闭时运行
	$("#close_program").empty()
	for(var i = 0 ;i < response["关闭时运行"].length; i++)
	{
		var parameter = response["关闭时运行"][i];
		$("#close_program").append('<li class="list-group-item" data-section="关闭时运行">' + parameter + remove_section + '</li>');
	}

	$(".remove_section").click(function() {
		var parameter = {}
		parameter.section = $(this).parent().data("section")
		parameter.value = $(this).parent().text()

		AJAX("del_section", parameter, function(response){
			get_setting();
		});
		return false;
	});

	// 鼠标手势开关
	$('#MouseGesture').attr("checked",response["鼠标手势开关"]["启用"]=="1");
	$('#GestureTrack').attr("checked",response["鼠标手势开关"]["轨迹"]=="1");
	$('#GestureResult').attr("checked",response["鼠标手势开关"]["动作"]=="1");

	// 鼠标手势列表
	var remove_td = '<td style="vertical-align:middle"><a href="#" class="remove-mouse"><span class="glyphicon glyphicon-remove"></span></a></td>'
	$("#MouseGestureList").empty()
	for(var i = 0 ;i < response["鼠标手势"].length; i++)
	{
		var line = response["鼠标手势"][i];
		line = line.split("=", 2)
		var key = line[0]
		if(/^([↑↓←→]+)$/.test(key))
		{
			var value = line[1].split("|", 2)
			var name = value[0]
			var action = value[1]

			var tr = $("<tr class='exist'></tr>")

			var td = $('<td class="direction" style="vertical-align:middle"></td>')
			td.text(key)
			tr.append(td)

			var td = $('<td><input type="text" class="form-control name"></td>')
			td.find("input").val(name)
			tr.append(td)

			var td = $('<td><input type="text" class="form-control action"></td>')
			td.find("input").val(action)
			tr.append(td)

			tr.append(remove_td)

			$("#MouseGestureList").append(tr)
		}
		// $("#close_program").append('<li class="list-group-item">' + parameter + remove + '</li>');
	}

	var add_tr = $('<tr>\
	<td><input type="text" class="form-control direction" placeholder="输入手势的方向"></td>\
	<td><input type="text" class="form-control name" placeholder="输入手势的名称"></td>\
	<td><input type="text" class="form-control action" placeholder="输入手势的动作（模拟按键）"></td>\
	<td style="vertical-align:middle"><a href="#" class="add-mouse"><span class="glyphicon glyphicon-plus"></span></a></td>\
	</tr>')
	$("#MouseGestureList").append(add_tr)


	$(".exist :input").on('input', function() {
		var parameter = {}
		parameter.section = "鼠标手势"
		parameter.name = $(this).parents("tr").find(".direction").text()
		parameter.value = $(this).parents("tr").find(".name").val() + "|" + $(this).parents("tr").find(".action").val()

		AJAX("set_setting", parameter, function(response){
		});
	});


	$(".remove-mouse").click(function() {
		var direction = $(this).parents("tr").find(".direction").text()
		var parameter = {}
		parameter.section = "鼠标手势"
		parameter.name = direction

		AJAX("del_setting", parameter, function(response){
			get_setting();
		});
		return false;
	});

	$(".add-mouse").click(function() {
		var direction = $(this).parents("tr").find(".direction").val()
		if(/^([↑↓←→]+)$/.test(direction))
		{
			var parameter = {}
			parameter.section = "鼠标手势"
			parameter.name = direction
			parameter.value = $(this).parents("tr").find(".name").val() + "|" + $(this).parents("tr").find(".action").val()

			AJAX("set_setting", parameter, function(response){
				get_setting();
			});
		}
		else
		{
			alert("您输入的方向不正确")
		}
		return false;
	});
}

function get_setting()
{
	AJAX("get_setting", {}, function(response){
		Render(response);
	})
}

$(document).ready(function() {
	console.log("不想要本宝宝的，在[基本设置]写上：人家不要WEB啦=1")
	$('form,input,select,textarea').attr("autocomplete", "off");

	get_setting();

	$(".kv").on('input', function(e){
		var parameter = {}
		parameter.section = $(this).data("section")
		parameter.name = $(this).data("name")
		parameter.value = $(this).val()

		AJAX("set_setting", parameter, function(response){
		});
	});

	$(":checkbox").change(function() {

		var parameter = {}
		parameter.section = $(this).data("section")
		parameter.name = $(this).data("name")
		parameter.value = this.checked ? "1" : "0"

		AJAX("set_setting", parameter, function(response){
		});
	});

	$(".add_section").click(function() {
		var input = $(this).parents(".input-group").find("input")
		var parameter = {}
		parameter.section = $(this).data("section")
		parameter.value = input.val()
		if(parameter.value=="") return false;

		AJAX("add_section", parameter, function(response){
			input.val("")
			get_setting();
		});

		return false;
	});
});