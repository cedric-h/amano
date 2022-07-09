class Shader {
  constructor(program) {
    this.program = program
  }
}

class Buffer {
  constructor(ib, vb, indexCount) {
    this.ib = ib
    this.vb = vb
    this.indexCount = indexCount
  }
}

class Attrib {
  constructor(name, size) {
    this.name = name
    this.size = size
  }
}

function logGLCall(functionName, args) {   
  console.log("gl." + functionName + "(" + WebGLDebugUtils.glFunctionArgsToString(functionName, args) + ")");   
} 

class Renderer {
  constructor(canvas) {
    let gl = canvas.getContext('webgl');

    if (!gl) {
      console.log('falling back to experimental webgl');
      gl = canvas.getContext('expreimental-webgl');
    }

    this.gl_ex_lose_context = gl.getExtension("WEBGL_lose_context")
    this.gl = gl

    gl.enable(gl.DEPTH_TEST);
    gl.enable(gl.CULL_FACE);
    gl.frontFace(gl.CCW);
    gl.cullFace(gl.BACK);

    canvas.addEventListener('webglcontextlost', (evnt) => {
      console.log("WebGL context lost, restoring");
      this.gl_ex_lose_context.restoreContext();
    });
  }

  createShader(vertexText, fragmentText) {
    var vertexShader = this.gl.createShader(this.gl.VERTEX_SHADER);
    var fragmentShader = this.gl.createShader(this.gl.FRAGMENT_SHADER);
    
    this.gl.shaderSource(vertexShader, vertexText);
    this.gl.shaderSource(fragmentShader, fragmentText);
    
    this.gl.compileShader(vertexShader);
    if (!this.gl.getShaderParameter(vertexShader, this.gl.COMPILE_STATUS))
    {
      console.error("failed compiling vertex shader", this.gl.getShaderInfoLog(vertexShader));
      return null;
    }
      
    this.gl.compileShader(fragmentShader);
    if (!this.gl.getShaderParameter(fragmentShader, this.gl.COMPILE_STATUS))
    {
      console.error("failed compiling fragment shader", this.gl.getShaderInfoLog(fragmentShader));
      return null;
    }

    var program = this.gl.createProgram();
    this.gl.attachShader(program, vertexShader);
    this.gl.attachShader(program, fragmentShader);
    
    this.gl.linkProgram(program);
    if (!this.gl.getProgramParameter(program, this.gl.LINK_STATUS))
    {
      console.error('error linking program', this.gl.LINK_STATUS);
      return null;
    }

    this.gl.validateProgram(program);
    if (!this.gl.getProgramParameter(program, this.gl.VALIDATE_STATUS))
    {
      console.error('error validating program', this.gl.VALIDATE_STATUS);
      return null;
    }

    return new Shader(program);
  }

  updateBuffer(buffer, vertices, indices) {
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, buffer.vb);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, vertices, this.gl.DYNAMIC_DRAW);

    this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, buffer.ib);
    this.gl.bufferData(this.gl.ELEMENT_ARRAY_BUFFER, indices, this.gl.DYNAMIC_DRAW);

    buffer.indexCount = indices.length;
  }

  createBuffer(vertices, indices) {
    let buf = new Buffer(this.gl.createBuffer(), this.gl.createBuffer(), indices.length)
    this.updateBuffer(buf, vertices, indices)
    return buf
  }

  vertexAttribFloatDesc(shader, attribs) {
    let vertexSize = 0;

    for (let i in attribs) {
      vertexSize += attribs[i].size;
    }

    let offset = 0;

    for (let i in attribs) {
      let location = this.gl.getAttribLocation(shader.program, attribs[i].name);
      this.gl.vertexAttribPointer(location, attribs[i].size, this.gl.FLOAT, false, vertexSize * Float32Array.BYTES_PER_ELEMENT, offset);
      this.gl.enableVertexAttribArray(location);
      offset += attribs[i].size * Float32Array.BYTES_PER_ELEMENT;
    }
  }

  setUniformMatrix4fv(shader, name, value) {
    this.gl.useProgram(shader.program)
    this.gl.uniformMatrix4fv(this.gl.getUniformLocation(shader.program, name), false, value)
  }

  draw(shader, buffer) {
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, buffer.vb);
    this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, buffer.ib);
    this.gl.useProgram(shader.program);
    this.gl.drawElements(this.gl.TRIANGLES, buffer.indexCount, this.gl.UNSIGNED_SHORT, 0);
  }
}
